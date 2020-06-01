//
// Created by 刘立悟 on 2020/5/18.
//

#include "client.h"
#include "config.h"
#include "log.h"

using namespace tLBS;

DEFINE_int32(max_clients, 10000, "最大的同事产生的客户端连接数");

std::map<uint64_t, Client *> Client::clients;

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

Client* Client::create(Connection *conn, int flags) {
    auto mapIter = clients.find(conn->getFd());
    if (mapIter == clients.end()) {
        clients[conn->getFd()] = new Client(conn, flags);
        conn->setData(clients[conn->getFd()]);
    }
    return clients[conn->getFd()];
}

Client::Client(Connection *conn, int flags) {
    this->flags = 0;
    this->conn = conn;
    this->flags |= flags;
    this->id = conn->getFd();
    this->name = nullptr;
    info("创建一个client#") << this->id;
    // 向connection安装client的读句柄
    conn->setReadHandler(connReadHandler);
}

Object * Client::getName() {
    return this->name;
}

void Client::free(tLBS::Client *client) {
    auto mapIter = Client::clients.find(client->getId());
    if (mapIter != Client::clients.end()) {
        Client::clients.erase(client->getId());
    }
}

Client::~Client() {
    info("销毁client#") << this->getId();
}

Connection* Client::getConnection() {
    return this->conn;
}


void Client::readFromConnection() {
    char *queryBuf = (char *)malloc(sizeof(char) * (1024 * 1024 * 32)); // 32M
    int nRead = conn->read(queryBuf, sizeof(queryBuf));
    info("client#") << this->getId()
        << "从connection中读取出" << nRead << "个字符: " << queryBuf;

    ::free(queryBuf);
}

void Client::connReadHandler(Connection *data) {
    auto *conn = (Connection *)data;
    auto *client = (Client *)conn->getData();
    client->readFromConnection();
}


