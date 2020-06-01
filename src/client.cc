//
// Created by 刘立悟 on 2020/5/18.
//

#include "client.h"
#include "config.h"
#include "log.h"

using namespace tLBS;

DEFINE_int32(max_clients, 10000, "最大的同事产生的客户端连接数");

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



Client* Client::getClient(uint64_t clientId) {
    return clients[clientId];
}

uint64_t Client::getId() {
    return this->id;
}

uint64_t Client::getFlags() {
    return this->flags;
}

Client::Client(Connection *conn, int flags) {
    this->flags = 0;
    this->conn = conn;
    this->flags |= flags;
    this->id = ++nextClientId;
    conn->setData(this);
    info("创建一个client#") << this->id;
    // 向connection安装client的读句柄
    conn->setReadHandler(connReadHandler);
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
    info("销毁client#") << this->getId();
}

Connection* Client::getConnection() {
    return this->conn;
}


void Client::readFromConnection() {
    int strLen = (1024 * 1024 * 32); // 32M
    char *qb = (char *)malloc(sizeof(char) * strLen);
    memset(qb, 0, sizeof(char) * strLen);
    int nRead = conn->read(qb, strLen);

    if (nRead == -1) {
        if (conn->getState() == ConnectionState::CONN_STATE_CONNECTED) {
            return;
        }
        else {
            error("connection#") << conn->getFd()
                << "读取数据错误: " << strerror(conn->getLastErrno());
            return;
        }
    }
    else if (nRead == 0) {
        warning("connection输入为0,client#") << this->getId()
            << " 关闭connection#" << conn->getFd();
        conn->close();
        return;
    }

    std::string q = qb;
    ::free(qb);
    // 去掉末尾的\r\n \t
    size_t n = q.find_last_not_of("\r \n\t");
    q.erase(n + 1, q.size() - n);

    n = q.find_first_not_of(" \r\n\t");
    q.erase(0, n);

    const char *queryBuf = q.c_str();
//    for (int i = 0; i < strlen(queryBuf); i++) {
//        char msg[1024];
//        snprintf(msg, sizeof(msg), "%d -- %d : %c", i, *(queryBuf + i), *(queryBuf + i));
//        info(msg);
//    }
    info("client#") << this->getId()
        << "从connection中读取出" << strlen(queryBuf) // \r\n
        << "个字符: " << queryBuf;
    this->queryBuf = queryBuf;

//    const char *resp = "+OK\r\n";
//    conn->write(resp, strlen(resp));
}

void Client::connReadHandler(Connection *data) {
    auto *conn = (Connection *)data;
    auto *client = (Client *)conn->getData();
    client->readFromConnection();
}


