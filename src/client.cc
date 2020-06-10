//
// Created by 刘立悟 on 2020/5/18.
//

#include "client.h"
#include "connection.h"
#include "log.h"

using namespace tLBS;

std::map<uint64_t, Client *> Client::clients;

Client::Client(tLBS::Connection *conn) {
    this->conn = conn;
}

Client::~Client() {
    if (this->conn != nullptr) {
        delete this->conn;
    }
}

Connection * Client::getConnection() {
    return this->conn;
}

void Client::linkClient(Client *client) {
    clients[client->getConnection()->getId()] = client;
}

void Client::unlinkClient(Client *client) {
    auto mapIter = clients.find(client->getConnection()->getId());
    if (mapIter != clients.end()) {
        clients.erase(mapIter);
    }
    delete client;
}


void Client::unlinkIfNeed() {
    std::vector<Client *> cv;
    for (auto mapIter = clients.begin(); mapIter != clients.end(); mapIter++) {
        if (mapIter->second->getConnection()->isPendingClose()) {
            cv.push_back(mapIter->second);
        }
    }
    if (cv.size() > 0) {
        warning("即将关闭的连接数: ") << cv.size();
    }
    for (int i = 0; i < cv.size(); i++) {
        auto client = cv[i];
        client->getConnection()->close();
        unlinkClient(client);
    }
}

int Client::execQuit(tLBS::Connection *conn, std::vector<std::string> args) {
    UNUSED(args);
    const char *resp = "+OK 👋啊朋友再见，啊朋友再见，啊朋友再见吧再见吧~再见吧!👋";
    conn->success(resp);
    conn->pendingClose();
    return C_ERR;
}


int Client::execHello(tLBS::Connection *conn, std::vector<std::string> args) {
    UNUSED(args);
    conn->success("+OK Hello! 你好啊!~👋");
    return C_OK;
}
