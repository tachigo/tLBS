//
// Created by 刘立悟 on 2020/5/18.
//

#include "client.h"
#include "connection.h"

using namespace tLBS;

int Client::execQuit(tLBS::Connection *conn, std::vector<std::string> args) {
    UNUSED(args);
    const char *resp = "👋啊朋友再见，啊朋友再见，啊朋友再见吧再见吧~再见吧!👋";
    conn->success(resp);
    conn->pendingClose();
    return C_ERR;
}


int Client::execHello(tLBS::Connection *conn, std::vector<std::string> args) {
    UNUSED(args);
    conn->success("Hello! 你好啊!~👋");
    return C_OK;
}
