//
// Created by 刘立悟 on 2020/5/18.
//
#include "common.h"
#include "config.h"
#include "log.h"
#include "server.h"
#include "net_tcp.h"
#include "el.h"

using namespace tLBS;

int main(int argc, char *argv[]) {
    int j;
    Config::init(&argc, &argv); // 根据命令行参数初始化配置
    Log::init(argv[0]); // 初始化日志
    warning("oO0OoO0OoO0Oo tLBS-server is starting oO0OoO0OoO0Oo");

    auto *server = new Server();
    server->init(); // 初始化服务器

    // i/o多路复用代理
    auto *el = new EventLoop(10);
    info("创建i/o多路复用处理对象: " + el->getName());

    auto *net = new NetTcp();
    net->listenPort(); // 建立tcp网络监听

    int *sockFds = net->getTcpFd();
    int sockFdCount = net->getTcpFdCount();
    for (j = 0; j < sockFdCount; j++) {
        // 将监听tcp的文件描述符注册到事件循环中
        if (el->addFileEvent(sockFds[j], EL_READABLE) != EL_OK) {
            fatal("创建tcp套接字i/o事件失败");
        }
    }
    el->start();
    return C_OK;
}