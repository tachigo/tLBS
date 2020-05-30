//
// Created by 刘立悟 on 2020/5/18.
//

#include "server.h"
#include "config.h"
#include "log.h"

#include <string>
#include <fstream>

using namespace tLBS;

DEFINE_string(pid_file, "/var/run/tLBS-server.pid", "PID进程锁文件");
DEFINE_bool(daemonize, false, "是否以守护进程方式启动");


Server::Server() {
    info("创建server对象");
    this->pid = ::getpid();
    this->shutdownAsap = 0;
    this->archBits = (sizeof(long) == 8) ? 64 : 32;
}

Server::~Server() {
    info("析构server对象");
}

pid_t Server::getPid() {
    return this->pid;
}

int Server::getShutdownAsap() {
    return this->shutdownAsap;
}

int Server::getArchBits() {
    return this->archBits;
}

void Server::daemonize() {
    info("将进程变成守护进程");
    if (fork() != 0) {
        // 父
        exit(0);
    }
    // 子 脱离会话
    setsid();
}

void Server::init() {
    info("server对象初始化");
    if (FLAGS_daemonize) {
        this->daemonize();
        this->createPidFile();
    }
}

void Server::createPidFile() {
    info("创建PID进程锁文件");
    std::string pid_file = FLAGS_pid_file;
    std::ofstream ofs;
    ofs.open(pid_file, std::ios::out);
    ofs << this->pid << std::endl;
}

