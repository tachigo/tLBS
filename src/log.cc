//
// Created by liuliwu on 2020-05-28.
//

#include "log.h"
#include <memory>

tLBS::Log::Log(const char *programName) {
    google::InitGoogleLogging(programName);
    FLAGS_log_dir = "../log";
    FLAGS_log_prefix = true;
    FLAGS_alsologtostderr = true;
    FLAGS_colorlogtostderr = true;
    FLAGS_stop_logging_if_full_disk = true;
    FLAGS_max_log_size = 100; // 100M
    info("初始化google日志对象");
}

tLBS::Log::~Log() {
    info("google日志对象析构");
    google::ShutdownGoogleLogging();
}

void tLBS::Log::init(const char *programName) {
    new Log(programName);
}