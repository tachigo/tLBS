//
// Created by 刘立悟 on 2020/5/18.
//
#include "common.h"
#include "config.h"
#include "log.h"
#include "server.h"
#include "net_tcp.h"
#include "el.h"
#include "threadpool_c.h"
#include "command.h"
#include "http.h"
#include "db.h"

#include <csignal>

using namespace tLBS;


void beforeEventLoopSleep() {
    Server::beforeEventLoopSleep();

}

int main(int argc, char *argv[]) {
    int j;
    Log::init(argv[0]); // 初始化日志
    atexit(Log::free);
    Config::init(&argc, &argv); // 根据命令行参数初始化配置
    atexit(Config::free);

    Server *server = Server::getInstance();
    atexit(Server::free);
    server->init(); // 初始化服务器
    server->setExecutable(getAbsolutePath(argv[0]));
    Command::init();
    atexit(Command::free);
    Http::init();
    atexit(Http::free);

    warning("👋👋👋👋👋👋👋👋 Hello! tLBS-SERVER~ 👋👋👋👋👋👋👋👋");
    warning("executable: ") << server->getExecutable();
    warning("pid: ") << server->getPid();
    warning("arch bits: ") << server->getArchBits();
    if (FLAGS_config_file.size() > 0) {
        warning("config file: ") << FLAGS_config_file;
    }
    // 初始化线程池
    sigset_t signal_mask;
    sigemptyset (&signal_mask);
    sigaddset(&signal_mask, SIGPIPE);
    pthread_sigmask(SIG_BLOCK, &signal_mask, nullptr);
    // 1.初始化client线程池
    ThreadPool::createPool("client", 100);
//     2.初始化data线程池
//    ThreadPool::createPool("data", 1);
    atexit(ThreadPool::free);
    // 初始化db
    Db::init();
    atexit(Db::free);
    // i/o多路复用代理
    Connection::adjustMaxConnections();
    EventLoop *el = EventLoop::create(FLAGS_max_connections);
    atexit(EventLoop::free);
    warning("i/o多路复用: " + el->getName());
//    if (el->addTimeEvent(1, Server::timeEventCron, nullptr) == EL_ERR) {
//        fatal("添加server定时任务失败");
//    }
    ThreadPool::createSingleThread(nullptr, Server::threadCron, nullptr);

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

    el->setBeforeSleep(beforeEventLoopSleep);
    el->start();
    return C_OK;
}