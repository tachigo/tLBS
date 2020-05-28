//
// Created by liuliwu on 2020-05-28.
//

#ifndef TLBS_CONFIG_H
#define TLBS_CONFIG_H

#include <gflags/gflags.h>

DEFINE_string(config_file, nullptr, "配置文件");
DEFINE_bool(daemonize, false, "是否是守护进程方式启动");
DEFINE_int32(dbnum, 10, "数据库的数量");
DEFINE_int32(tcp_port, 8899, "tcp端口号");

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
