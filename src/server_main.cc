//
// Created by 刘立悟 on 2020/5/18.
//
#include "common.h"
#include "config.h"
#include "log.h"
#include "server.h"
#include "net_tcp.h"
#include "el.h"
#include "client.h"

using namespace tLBS;

int main(int argc, char *argv[]) {
    int j;
    Config::init(&argc, &argv); // 根据命令行参数初始化配置
    Log::init(argv[0]); // 初始化日志
    atexit(Log::free);
    warning("oO0OoO0OoO0Oo tLBS-server is starting oO0OoO0OoO0Oo");

    Server server = Server();
    server.init(); // 初始化服务器
    warning("pid: ") << server.getPid();
    warning("arch bits: ") << server.getArchBits();
    // i/o多路复用代理
    Client::adjustMaxClients();
    EventLoop el = EventLoop(FLAGS_max_clients + FD_SET_INCR);
    info("创建i/o多路复用处理对象: " + el.getName());

    NetTcp net = NetTcp();
    net.bindAndListen(); // 建立tcp网络地址监听

    int *tcpFds = net.getTcpFd();
    int tcpFdCount = net.getTcpFdCount();
    info("监听tcp网络的文件描述符数: ") << tcpFdCount;
    for (j = 0; j < tcpFdCount; j++) {
        // 将监听接受tcp连接时的处理句柄注册到事件循环中
        if (el.addFileEvent(tcpFds[j], EL_READABLE, NetTcp::acceptReader, nullptr) != EL_OK) {
            fatal("将监听接受tcp连接时的处理句柄注册到事件循环中");
        }
    }
    el.start();
    return C_OK;
}