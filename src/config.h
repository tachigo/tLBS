//
// Created by liuliwu on 2020-05-28.
//

#ifndef TLBS_CONFIG_H
#define TLBS_CONFIG_H

#include <gflags/gflags.h>
#include <string>

// server config
DECLARE_string(pid_file); // PID进程锁文件
DECLARE_bool(daemonize); // 是否以守护进程启动

// net tcp
DECLARE_int32(tcp_port); // tcp端口号

namespace tLBS {
    class Config {
    private:
        static Config *instance;
        Config();

    public:
        static void init(int *argc, char*** argv);
        static Config *getInstance();
        ~Config() = default;
    };

}


#endif //TLBS_CONFIG_H
