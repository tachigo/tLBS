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
#include "threadpool_c.h"

using namespace tLBS;


void *testThread(void *arg) {
    static int index = 0;
    info("线程执行 [") << "[" << pthread_self() << "]#" << ++index;
    return (void *)0;
}

int main(int argc, char *argv[]) {
    int j;
    Config::init(&argc, &argv); // 根据命令行参数初始化配置
    atexit(Config::free);
    Log::init(argv[0]); // 初始化日志
    atexit(Log::free);

    Server *server = Server::getInstance();
    atexit(Server::free);
    server->init(); // 初始化服务器
    warning("<====!!!!====> Hello! tLBS-SERVER~ <====!!!!====>");
    warning("pid: ") << server->getPid();
    warning("arch bits: ") << server->getArchBits();
    // 初始化线程池
    // 1.初始化主要的线程池
//    ThreadPool::createPool("main", 10, 100, 5);
//    atexit(ThreadPool::destroyPools);
//    for (j = 0; j < 100; j++) {
//        ThreadPool::getPool("main")->enqueueTask(testThread, nullptr);
//    }
    // i/o多路复用代理
    Client::adjustMaxClients();
    EventLoop *el = EventLoop::create(FLAGS_max_clients + FD_SET_INCR);
    atexit(EventLoop::free);
    warning("i/o多路复用: " + el->getName());
    if (el->addTimeEvent(1, Server::cron, nullptr) == EL_ERR) {
        fatal("添加server定时任务失败");
    }

    NetTcp *net = NetTcp::getInstance();
    atexit(NetTcp::free);
    net->bindAndListen(); // 建立tcp网络地址监听

    int *tcpFds = net->getTcpFd();
    int tcpFdCount = net->getTcpFdCount();
    warning("监听tcp网络的文件描述符数: ") << tcpFdCount;
    for (j = 0; j < tcpFdCount; j++) {
        // 将监听接受tcp连接时的处理句柄注册到事件循环中
        if (el->addFileEvent(tcpFds[j], EL_READABLE, NetTcp::acceptHandler, nullptr) != EL_OK) {
            fatal("将监听接受tcp连接时的处理句柄注册到事件循环中");
        }
    }
    el->start();
    return C_OK;
}