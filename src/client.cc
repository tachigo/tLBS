//
// Created by 刘立悟 on 2020/5/18.
//

#include "client.h"
#include "config.h"
#include "log.h"
#include "command.h"
#include <cctype>

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

void Client::setFlags(uint64_t flags) {
    this->flags = flags;
}

void Client::pendingClose() {
    this->flags |= CLIENT_FLAGS_PENDING_CLOSE;
}

Client::Client(Connection *conn, int flags) {
    this->flags = 0;
    this->conn = conn;
    this->flags |= flags;
    this->id = ++nextClientId;
    this->format = ClientFormat::CLIENT_FORMAT_LEGACY;
    conn->setData(this);
    info("创建一个client#") << this->id;
    // 向connection安装client的读句柄
    conn->setReadHandler(connReadHandler);
}

ClientFormat Client::getFormat() {
    return this->format;
}

void Client::setFormat(tLBS::ClientFormat format) {
    this->format = format;
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
    long long start = ustime();
    int segLen = (1024); // 32M
    int totalRead = 0;
    std::string qb;
    while (true) {
        // 每次读出一部分
        char buf[segLen];
        int nRead = conn->read(buf, sizeof(char) * segLen);
//        info("读出") << nRead << "个字符: " << buf;
        if (nRead == -1) {
            if (conn->getState() == ConnectionState::CONN_STATE_CONNECTED) {
                return;
            }
            else if (conn->getLastErrno() != 0) {
                error("connection#") << conn->getFd() << "读取数据错误: " << strerror(conn->getLastErrno());
                return;
            }
            else {
                break;
            }
        }
        else if (strlen(buf) > 0) {
//            dumpString(buf);
            buf[segLen] = '\0'; // 确保最后一个字符是\0
//            dumpString(buf);
            // 有新的内容 追加进去
            totalRead += nRead;
            qb += buf;
//            info("汇总字符数: ") << qb.size();
//            info("汇总字符串: ") << qb;
            if (nRead == 0) {
                break;
            }
        }
    }
    if (totalRead == 0) {
        warning("connection输入为0,client#") << this->getId() << " 关闭connection#" << conn->getFd();
        conn->close();
        return;
    }

    // 去掉首尾的\r\n \t
    size_t n = qb.find_last_not_of("\r \n\t");
    qb.erase(n + 1, qb.size() - n);

    n = qb.find_first_not_of(" \r\n\t");
    qb.erase(0, n);
    this->query = qb;

    info("client#") << this->getId()
        << "从connection中读取出" << this->query.size()
        << "个字符: " << this->query;
    this->args.clear();
    Client::parseCommandLine(this->query.c_str(), &this->args);

//    for (int i = 0; i < (int)this->args.size(); i++) {
//        info("argv#") << (i + 1);
//        dumpString(this->args[i].c_str());
//    }
    if (this->args.size() > 0) {
        if (processCommandAndReset() == C_OK) {
            long long duration = ustime() - start;
            char msg[128];
            sprintf(msg, "命令[%s]外部执行时间: %0.5f 毫秒", this->args[0].c_str(), (double)duration / (double)1000);
            info(msg);
        }
    }
}

int Client::processCommand() {
    info("执行命令: ") << this->args[0];
    Command *command = Command::findCommand(this->args[0]);
    if (command == nullptr) {
        // 没有找到命令
        const char *resp = "未知的命令!!\r\n";
        conn->write(resp, strlen(resp));
        return C_ERR;
    }
    long long start = ustime();
    int ret = command->call(this);
    long long duration = ustime() - start;
    char msg[128];
    sprintf(msg, "命令[%s]内部执行时间: %0.5f 毫秒", this->args[0].c_str(), (double)duration/(double)1000);
    info(msg);
    return ret;
}

int Client::processCommandAndReset() {
    if (processCommand() == C_OK) {
        // reset client
        return C_OK;
    }
    return C_ERR;
}

void Client::connReadHandler(Connection *data) {
    auto *conn = (Connection *)data;
    auto *client = (Client *)conn->getData();
    client->readFromConnection();
}


void Client::parseCommandLine(const char *line, std::vector<std::string> *argv) {
    const char *p = line;
    std::string current;
    while (true) {
        while ((*p && isspace(*p)) || *p < 0) {
            // 如果是空格或者不正确的ascii码，指针向前进1
            p++;
        }
        if (*p) {
            // 有非空格字符
            bool inQuotes = false;
            bool inSingleQuotes = false;
            bool done = false;
            while (!done) {
                if (inQuotes) {
                    // 双引号中
                    if (*p == '\\' && *(p+1) == 'x' && isHexDigit(*(p+2)) && isHexDigit(*(p+3))) {
                        unsigned char byte;
                        byte = (hexDigit2int(*(p+2))*16)+
                               hexDigit2int(*(p+3));
                        current += (char *)&byte;
//                        info("8进制数: ") << current;
                        p += 3;
                    }
                    else if (*p == '\\' && *(p+1)) {
                        char c;
                        p++;
                        switch(*p) {
                            case 'n': c = '\n'; break;
                            case 'r': c = '\r'; break;
                            case 't': c = '\t'; break;
                            case 'b': c = '\b'; break;
                            case 'a': c = '\a'; break;
                            default: c = *p; break;
                        }
                        current += &c;
//                        info("转义字符: ") << *p;
                    }
                    else if (*p == '"') {
                        if (*(p+1) && !isspace(*(p+1))) {
                            return;
                        }
                        done = true;
//                        info("双引号结束: ") << *p;
                    }
                    else if (!*p) {
                        return;
                    }
                    else {
                        current += *p;
//                        info("默认情况: ") << *p;
                    }
                }
                else if (inSingleQuotes) {
                    if (*p == '\\' && *(p+1) == '\'') {
                        p++;
                        current += "'";
//                        info("正常情况: ") << *p;
                    } else if (*p == '\'') {
                        if (*(p+1) && !isspace(*(p+1))) {
                            return;
                        }
                        done = true;
//                        info("单引号结束: ") << current;
                    } else if (!*p) {
                        /* unterminated quotes */
                        return;
                    } else {
                        current += *p;
//                        info("正常情况: ") << *p;
                    }
                }
                else {
                    switch(*p) {
                        case ' ':
                        case '\n':
                        case '\r':
                        case '\t':
                        case '\0':
                            done = true;
//                            info("结束字符: ") << current;
                            break;
                        case '"':
                            inQuotes = true;
//                            info("进入双引号: ") << current;
                            break;
                        case '\'':
                            inSingleQuotes = true;
//                            info("进入单引号: ") << current;
                            break;
                        default:
                            current += *p;
//                            info("默认情况: ") << *p;
                            break;
                    }

                }
                if (*p) {
                    p++;
                }
            }
            argv->push_back(current);
//            info("argv: ") << current;
            current = "";
        }
        else {
            return;
        }
    }
}

std::vector<std::string> Client::getArgs() {
    return this->args;
}

std::string Client::arg(int i) {
    if (i >= this->args.size()) {
        return "";
    }
    return this->args[i];
}

int Client::formatSelect(tLBS::Client *client) {
    std::string format = client->arg(1);
    if (strcmp(format.c_str(), "json") == 0) {
        client->success();
        client->setFormat(ClientFormat::CLIENT_FORMAT_JSON);
        info("设置client返回数据格式为: json");
        return C_OK;
    }
    else if (strcmp(format.c_str(), "legacy") == 0) {
        client->success();
        client->setFormat(ClientFormat::CLIENT_FORMAT_LEGACY);
        info("设置client返回数据格式为: legacy");
        return C_OK;
    }
    else {
        client->success(
                client->getFormat() == ClientFormat::CLIENT_FORMAT_LEGACY ?
                "legacy" :
                R"({"errno": 0, "data": "json"})");
        return C_OK;
    }
}


int Client::success() {
    if (this->format == CLIENT_FORMAT_LEGACY) {
        return this->success("+OK");
    }
    else {
        return this->success(R"({"errno": 0, "data": "OK"})");
    }
}

int Client::success(const char *msg) {
    std::string str = msg;
    str += "\r\n";
    const char *data = str.c_str();
    conn->write((void *)data, strlen(data));
    return C_OK;
}

int Client::fail(int error, const char *msg) {
    if (this->format == CLIENT_FORMAT_LEGACY) {
        return this->fail("-ERR %d: %s", error, msg);
    }
    else {
        return this->fail(R"({"errno": %d, "data": "%s"})", error, msg);
    }
}

int Client::fail(const char *fmt, ...) {
    va_list ap;
    char msg[1024];
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    std::string str = msg;
    str += "\r\n";
    const char *data = str.c_str();

    conn->write((void *)data, strlen(data));
    return C_OK;
}


int Client::cron(long long id, void *data) {
    // 断开一些client
    std::vector<uint64_t > freeClients;
    for (auto mapIter = clients.begin(); mapIter != clients.end(); mapIter++) {
        Client *client = mapIter->second;
        int flags = client->getFlags();
        if (flags & CLIENT_FLAGS_PENDING_CLOSE) {
            freeClients.push_back(client->getId());
        }
    }
    for (int i = 0; i < freeClients.size(); i++) {
        uint64_t clientId = freeClients[i];
        Client *client = clients[clientId];
        clients.erase(clientId);
        delete client;
    }
    return C_OK;
}
