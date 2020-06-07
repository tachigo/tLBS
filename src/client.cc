//
// Created by 刘立悟 on 2020/5/18.
//

#include "client.h"
#include "config.h"
#include "log.h"
#include "command.h"
#include "db.h"
#include "http.h"
#include "threadpool_c.h"

using namespace tLBS;

DEFINE_int32(max_clients, 10000, "最大的同事产生的客户端连接数");
DEFINE_bool(threads_client, true, "是否使用线程处理客户端");

std::map<uint64_t, Client *> Client::clients;
_Atomic uint64_t Client::nextClientId = 0;

void Client::adjustMaxClients() {
    info("适配最大可打开文件数的限制");
    rlim_t maxFileNum = FLAGS_max_clients + MIN_REVERSED_FDS;
    struct rlimit limit;
    if (getrlimit(RLIMIT_NOFILE, &limit) == -1) {
        warning("无法获知当前系统的 NOFILE 的限制，假设是1024个来设置 max_clients");
        FLAGS_max_clients = 1024 - MIN_REVERSED_FDS;
    }
    else {
        rlim_t oldLimit = limit.rlim_cur;
        if (oldLimit < maxFileNum) {
            rlim_t bestLimit;
            int setResourceLimitErrno = 0;

            bestLimit = maxFileNum;
            while (bestLimit > oldLimit) {
                // 不断地尝试setrlimit 每次递减16
                limit.rlim_cur = bestLimit;
                limit.rlim_max = bestLimit;
                if (setrlimit(RLIMIT_NOFILE, &limit) != -1) {
                    break; // 设置成功了
                }
                // 记录设置错误
                setResourceLimitErrno = errno;
                if (bestLimit < 16) {
                    break; // 不能出现负值
                }
                bestLimit -= 16;
            }
            if (bestLimit < oldLimit) {
                bestLimit = oldLimit;
            }
            if (bestLimit < maxFileNum) {
                int oldMaxClients = FLAGS_max_clients;
                FLAGS_max_clients = bestLimit - MIN_REVERSED_FDS;
                if (bestLimit < MIN_REVERSED_FDS) {
                    fatal("系统当前的 'ulimit -n'为: ") << oldLimit
                        << "，不足以启动服务器，请修改系统的最大可打开的文件数量";
                    exit(1); // never reach
                }
                if (setResourceLimitErrno != 0) {
                    warning("服务无法设置可打开的最大文件数至: ") << maxFileNum
                        << ", 由于系统错误: " << strerror(setResourceLimitErrno);
                }
                info("原配置的max_clients为: ") << oldMaxClients
                    << ", 现被修改为: " << FLAGS_max_clients;
            }
            else {
                info("原可打开的最大文件数为: ") << oldLimit
                    << ", 被修改为: " << bestLimit;
            }
        }
    }
}

std::map<uint64_t, Client *> Client::getClients() {
    return Client::clients;
}

std::string Client::getInfo() {
    return this->info;
}

void Client::setHttp(bool http) {
    this->http = http;
}

bool Client::isHttp() {
    return this->http;
}

void Client::setResponse(std::string response) {
    this->response = response;
}

Client* Client::getClient(uint64_t clientId) {
    return clients[clientId];
}

uint64_t Client::getId() {
    return this->id;
}

uint64_t Client::getFlags() {
    return this->flags;
}

void Client::setFlags(uint64_t flags) {
    this->flags = flags;
}

void Client::pendingClose() {
    this->flags |= CLIENT_FLAGS_PENDING_CLOSE;
    conn->setReadHandler(nullptr);
}

Client::Client(Connection *conn, int flags) {
    this->flags = 0;
    this->conn = conn;
    this->flags |= flags;
    this->id = ++nextClientId;
    conn->setData(this);
    char buf[100];
    snprintf(buf, sizeof(buf) - 1, "client#%llu[fd:%d]", this->id, this->conn->getFd());
    this->info = buf;
    this->response = "";
    this->sent = 0;
    this->http = false;
    this->setDb(Db::getDb(0));
    info("创建") << this->getInfo();
    // 向connection安装client的读句柄
    conn->setReadHandler(connReadHandler);
}

void Client::setDb(tLBS::Db *db) {
    this->db = db;
}

Db* Client::getDb() {
    return this->db;
}

void Client::link(Client *client) {
    clients[client->getId()] = client;
    info("client池大小: ") << clients.size();
}

void Client::free(Client *client) {
    auto mapIter = Client::clients.find(client->getId());
    if (mapIter != Client::clients.end()) {
        Client::clients.erase(client->getId());
        delete client;
    }
    info("client池大小: ") << clients.size();
}

Client::~Client() {
    if (this->conn != nullptr) {
        this->conn->close();
        Connection::free(this->conn);
    }
    info("销毁") << this->getInfo();
}

Connection* Client::getConnection() {
    return this->conn;
}

void Client::setQuery(const char *query) {
    this->query = query;
}

const char *Client::getQuery() {
    return this->query;
}

void Client::writeToConnection() {
//    info("Client::writeToConnection(): ") << this->response;
    // 当有响应数据且没有被发送过
    if (this->response.size() > 0 && this->sent == 0) {
        conn->write(this->response.c_str(), this->response.size());
        // 记录上一次发送的数据量
        this->sent = this->response.size();
        // 清空响应数据
        this->response = "";
    }
    conn->setWriteHandler(nullptr);
}

void Client::readFromConnection() {
//    info(this->getInfo()) << "::readFromConnection()";
    this->response = "";
    this->sent = 0;

    int segLen = (1024); // 32M
    int totalRead = 0;
    std::string qb;
    while (true) {
        // 每次读出一部分
        char buf[segLen];
        memset(buf, 0, segLen);
        int nRead = conn->read(buf, sizeof(char) * segLen);
//        info("读出") << nRead << "个字符: " << buf;
        if (nRead == -1) {
            if (conn->getState() == ConnectionState::CONN_STATE_CONNECTED) {
                return;
            }
            else if (conn->getLastErrno() != 0) {
                error("读取数据") << conn->getInfo() << "错误: " << strerror(conn->getLastErrno());
                this->pendingClose();
                return;
            }
            else {
                break;
            }
        }
        else {
            if (nRead == 0) {
                if (strlen(buf) > 0) {
                    buf[segLen] = '\0'; // 确保最后一个字符是\0
                    // 有新的内容 追加进去
                    totalRead += nRead;
                    qb += buf;
                }
                else {
                    break;
                }
            }
            else if (strlen(buf) > 0) {
                buf[segLen] = '\0'; // 确保最后一个字符是\0
                // 有新的内容 追加进去
                totalRead += nRead;
                qb += buf;
            }
        }
    }
    if (totalRead == 0) {
        warning(conn->getInfo()) << "读取数据长度为0, " << this->getInfo() << "准备关闭";
        this->pendingClose();
        return;
    }

    // 去掉首尾的\r\n \t
//    qb = trimString(qb.c_str(), " \r\n\t");
    this->setQuery(qb.c_str());
//    this->query = qb.c_str();

//    info(this->getInfo()) << "从"
//        << conn->getInfo() << "中读取出" << strlen(this->query)
//        << "个字符: " << this->query;

    if (FLAGS_threads_client) {
        // 使用线程处理
        ThreadPool::getPool("client")
            ->enqueueTask(Client::threadProcess, (void *)new ThreadArg(this, qb), "client::threadProcess");
    }
    else {
        if (Http::clientIsHttp(this)) {
            // 如果是http协议
            this->setHttp(true);
            info(this->getInfo()) << "是http请求";
        }
        else {
            this->setHttp(false);
            info(this->getInfo()) << "不是http请求";
        }

//    for (int i = 0; i < (int)this->args.size(); i++) {
//        info("argv#") << i << ": " << this->args[i].c_str();
//    }

        if (!this->isHttp()) {
            Command::processCommandAndReset(this);
        }
        else {
            Http::processHttpAndReset(this);
        }
    }
}

Client::ThreadArg::ThreadArg(tLBS::Client *client, std::string query) {
    this->client = client;
    this->query = query;
}

Client * Client::ThreadArg::getClient() {
    return this->client;
}

std::string Client::ThreadArg::getQuery() {
    return this->query;
}

void * Client::threadProcess(void *arg) {
    auto threadArg = (ThreadArg *)arg;
    Client *client = threadArg->getClient();
    char *query = (char *)malloc(threadArg->getQuery().size() * sizeof(char));
    threadArg->getQuery().copy(query, threadArg->getQuery().size(), 0);
    client->setQuery(query);
//    auto client = (Client *)arg;
    if (Http::clientIsHttp(client)) {
        // 如果是http协议
        client->setHttp(true);
        info(client->getInfo()) << "是http请求";
    }
    else {
        client->setHttp(false);
        info(client->getInfo()) << "不是http请求";
    }

//    for (int i = 0; i < (int)client->getArgs().size(); i++) {
//        info("argv#") << i << ": " << client->arg(i);
//    }

    if (!client->isHttp()) {
        Command::processCommandAndReset(client);
    }
    else {
        Http::processHttpAndReset(client);
    }
    return (void *)0;
}

void Client::connReadHandler(Connection *data) {
    auto *conn = (Connection *)data;
    auto *client = (Client *)conn->getData();
     client->readFromConnection();
}

void Client::connWriteHandler(Connection *data) {
    auto *conn = (Connection *)data;
    auto *client = (Client *)conn->getData();
    client->writeToConnection();
}


std::vector<std::string> Client::getArgs() {
    return this->args;
}

void Client::setArgs(std::vector<std::string> args) {
    this->args = args;
}

std::string Client::arg(int i) {
    if (i >= this->args.size()) {
        return "";
    }
    return this->args[i];
}

int Client::success() {
    return this->success(R"({"errno": 0, "data": "OK"})");

}

void Client::setSent(int sent) {
    this->sent = sent;
}

int Client::getSent() {
    return this->sent;
}

int Client::success(tLBS::Json *json) {
    int ret = this->success(json->toString().c_str());
    delete json;
    return ret;
}

int Client::success(const char *msg) {
    std::string str = msg;
    if (this->isHttp()) {
        std::string responseHeader = "HTTP/1.0 200 OK\r\nConnection: keep-alive\r\nContent-Type: application/json;charset=utf-8\r\nContent-Length: ";
        responseHeader += std::to_string(str.size());
        responseHeader += "\r\n\r\n";
        str = responseHeader + str;
//        conn->write(str.c_str(), str.size());
//        conn->close();
//        return C_OK;
    }
    else {
        str += "\r\n";
        this->response = str;
    }
    this->response = str;

//    if (this->isHttp()) {
//        this->flags |= CLIENT_FLAGS_CLOSE_AFTER_REPLY;
//    }
    conn->setWriteHandler(connWriteHandler);
    return C_OK;
}

int Client::fail(int error, const char *msg) {
    return this->fail(R"({"errno": %d, "data": "%s"})", error, msg);
}

int Client::fail(tLBS::Json *json) {
    int ret = this->fail(json->toString().c_str());
    delete json;
    return ret;
}

int Client::fail(const char *fmt, ...) {
    va_list ap;
    char msg[1024];
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    std::string str = msg;
    if (this->isHttp()) {
        std::string responseHeader = "HTTP/1.0 200 OK\r\nConnection: keep-alive\r\nContent-Type: application/json;charset=utf-8\r\nContent-Length: ";
        responseHeader += std::to_string(str.size());
        responseHeader += "\r\n\r\n";
        str = responseHeader + str;
//        conn->write(str.c_str(), str.size());
//        conn->close();
//        return C_ERR;
    }
    else {
        str += "\r\n";
        this->response = str;
    }
    this->response = str;

//    if (this->isHttp()) {
//        this->flags |= CLIENT_FLAGS_CLOSE_AFTER_REPLY;
//    }
    conn->setWriteHandler(connWriteHandler);
    return C_ERR;
}


int Client::cron(long long id, void *data) {
    // 断开一些client
    std::vector<Client *> freeClients;
    for (auto mapIter = clients.begin(); mapIter != clients.end(); mapIter++) {
        Client *client = mapIter->second;
        int flags = client->getFlags();
        if (flags & CLIENT_FLAGS_PENDING_CLOSE) {
            freeClients.push_back(client);
        }
        else if (flags & CLIENT_FLAGS_CLOSE_AFTER_REPLY) {
            if (client->getSent() > 0) {
                info(client->getInfo()) << "被设置为响应后关闭";
                freeClients.push_back(client);
            }
        }
    }
    for (int i = 0; i < freeClients.size(); i++) {
        free(freeClients[i]);
    }
    return C_OK;
}


int Client::execQuit(tLBS::Client *client) {
    const char *resp = "👋啊朋友再见，啊朋友再见，啊朋友再见吧再见吧~再见吧!👋";
    client->success(resp);
    uint64_t clientFlags = client->getFlags();
    clientFlags |= CLIENT_FLAGS_CLOSE_AFTER_REPLY;
    client->setFlags(clientFlags);
    return C_ERR;
}
