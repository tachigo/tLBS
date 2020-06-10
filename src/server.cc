//
// Created by 刘立悟 on 2020/5/18.
//

#include "server.h"
#include "config.h"
#include "log.h"
#include "el.h"
#include "db.h"
#include "client.h"

#include <string>
#include <fstream>
#include <csignal>

using namespace tLBS;

//DEFINE_string(pid_file, "tLBS-server.pid", "PID进程锁文件");
DEFINE_bool(daemonize, false, "是否以守护进程方式启动");
DEFINE_int32(server_hz, 10, "server时间事件每秒执行多少次");



Server *Server::instance = nullptr;

Server * Server::getInstance() {
    if (instance == nullptr) {
        instance = new Server();
    }
    return instance;
}

Server::Server() {
    this->pid = ::getpid();
    this->shutdownAsap = 0;
    this->archBits = (sizeof(long) == 8) ? 64 : 32;
    this->daemonized = FLAGS_daemonize;
    this->cronHz = FLAGS_server_hz;
    this->binRoot = FLAGS_bin_root;
    this->pidFile = std::string(getAbsolutePath("../run/")) + "tLBS-server#" + std::to_string(::getpid()) + ".pid";
    this->isParentProcess = true;
}

Server::~Server() {
    if (!this->isParentProcess || !FLAGS_daemonize) {
        info("销毁server对象");
    }
}


bool Server::getIsParentProcess() {
    return this->isParentProcess;
}

void Server::setExecutable(std::string executable) {
    this->executable = executable;
}

std::string Server::getExecutable() {
    return this->executable;
}

std::string Server::getBinRoot() {
    return this->binRoot;
}

pid_t Server::getPid() {
    return this->pid;
}

int Server::getShutdownAsap() {
    return this->shutdownAsap;
}

void Server::setShutdownAsap(int asap) {
    this->shutdownAsap = asap;
}

int Server::getArchBits() {
    return this->archBits;
}

void Server::daemonize() {
    if (fork() != 0) {
        // 父
        exit(0);
    }
    // 子 脱离会话
    this->isParentProcess = false;
    setsid();
    // 重新设置pid相关
    this->pid = ::getpid();
    this->pidFile = std::string(getAbsolutePath("../run/")) + "tLBS-server#" + std::to_string(::getpid()) + ".pid";
    info("将进程变成守护进程");
    warning("守护进程#") << this->pid;
}

void Server::init() {
    this->updateCachedTime();
    if (this->daemonized) {
        FLAGS_alsologtostderr = false;
        FLAGS_colorlogtostderr = false;
        this->daemonize();
        this->createPidFile();
    }
    // 信号处理
    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);

    struct sigaction act;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    act.sa_handler = Server::shutdown;
    sigaction(SIGTERM, &act, nullptr);
    sigaction(SIGINT, &act, nullptr);
}

std::string Server::getPidFile() {
    return this->pidFile;
}

void Server::createPidFile() {
    std::string pid_file = this->getPidFile();
    info("创建PID进程锁文件: ") << pid_file;
    std::ofstream ofs;
    ofs.open(pid_file, std::ios::out);
    ofs << this->pid << std::endl;
    ofs.close();
}

void Server::deletePidFile() {
    std::string pid_file = this->getPidFile();
    info("删除PID进程锁文件: ") << pid_file;
    unlink(pid_file.c_str());
}

void Server::shutdown(int sig) {
    std::string msg;
    switch (sig) {
        case SIGINT:
            msg = "接收到interrupt信号，server准备关闭...";
            break;
        case SIGTERM:
            msg = "接收到terminate信号，server准备关闭...";
            break;
        default:
            msg = "接收到shutdown信号，server准备关闭...";
    };

    warning(msg);
    Server::getInstance()->setShutdownAsap(1);
}


int Server::getCronHz() {
    return this->cronHz;
}

time_t Server::getUnixTime() {
    return this->unixTime;
}

long long Server::getUsTime() {
    return this->usTime;
}


void Server::updateCachedTime() {
    this->usTime = ustime();
    this->msTime = this->usTime / 1000;
    this->unixTime = this->msTime / 1000;
}


//void *Server::threadCron(void *arg) {
//    UNUSED(arg);
//    while (true) {
//        sleep(1);
////        info(pthread_self());
//        timeEventCron(0, nullptr);
//    }
//}


int Server::timeEventCron(long long id, void *data) {
    Server *server = getInstance();
    server->updateCachedTime();

    if (server->getShutdownAsap()) {
        // 确认要关闭
        if (prepareShutdown(SERVER_SHUTDOWN_NO_FLAGS) == C_OK) {
            exit(0); // 正常关闭
        }
        else {
            warning("server收到关闭信号，但是准备关闭时出错!");
            server->setShutdownAsap(0);
        }
    }

//    Db::cron(id, data);

    Client::unlinkIfNeed();

    return 1000 / server->getCronHz();
}

bool Server::isDaemonized() {
    return this->daemonized;
}


int Server::prepareShutdown(int flags) {
    UNUSED(flags);
    warning("确认准备关闭server...");

    Server *server = getInstance();
    if (server->isDaemonized()) {
        server->deletePidFile();
    }

    return C_OK;
}

void Server::free() {
    delete instance;
}

void Server::beforeEventLoopSleep() {
}