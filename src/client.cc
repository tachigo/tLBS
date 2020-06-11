//
// Created by 刘立悟 on 2020/5/18.
//

#include "client.h"
#include "connection.h"
#include "log.h"
#include "config.h"

using namespace tLBS;

int Client::execQuit(tLBS::Connection *conn, std::vector<std::string> args) {
    UNUSED(args);
    const char *resp = "+OK 👋啊朋友再见，啊朋友再见，啊朋友再见吧再见吧~再见吧!👋";
    conn->success(resp);
    conn->pendingClose();
    return C_ERR;
}


int Client::execHello(tLBS::Connection *conn, std::vector<std::string> args) {
    UNUSED(args);
    return conn->success("+OK Hello! 你好啊!~👋");
}


int Client::execPing(Connection *conn, std::vector<std::string> args) {
    UNUSED(args);
    char msg[1024];
    snprintf(msg, sizeof(msg) - 1, "pong %s:%s", FLAGS_tcp_host.c_str(), FLAGS_tcp_port.c_str());
    return conn->fail(msg);
}

int Client::execPong(Connection *conn, std::vector<std::string> args) {
    UNUSED(conn);
    UNUSED(args);
    // 不做任何处理
    return C_ERR;
}
