//
// Created by liuliwu on 2020-05-28.
//

#ifndef TLBS_CONFIG_H
#define TLBS_CONFIG_H

#include <gflags/gflags.h>

DEFINE_bool(daemonize, false, "是否是守护进程方式启动");

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
