//
// Created by liuliwu on 2020-05-28.
//

#include "log.h"
#include "config.h"
#include "server.h"

using namespace tLBS;

void Log::free() {
    Server *server = Server::getInstance();
    if (!server->getIsParentProcess() || !FLAGS_daemonize) {
        info("销毁log对象");
        warning("再见！~👋");
    }
    google::ShutdownGoogleLogging();
}

void Log::init(const char *programName) {
    google::InitGoogleLogging(programName);
    FLAGS_log_dir = "../log";
    FLAGS_log_prefix = true;
    FLAGS_alsologtostderr = true;
    FLAGS_colorlogtostderr = true;
    FLAGS_stop_logging_if_full_disk = true;
    FLAGS_max_log_size = 100; // 100M
//    info("初始化log对象");
}