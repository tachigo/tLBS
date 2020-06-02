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
DECLARE_int32(server_hz); // server时间事件频率 每秒多少次

// net tcp
DECLARE_string(tcp_port); // tcp端口号
DECLARE_int32(tcp_backlog); // tcp连接队列的长度
DECLARE_int32(tcp_keepalive); // tcp连接保持存活时长

// client
DECLARE_int32(max_clients); // 最大同时产生的客户端连接数
DECLARE_string(format); // client返回数据格式

namespace tLBS {
    class Config {
    private:
        static Config *instance;
        Config();

    public:
        static void init(int *argc, char*** argv);
        static void free();
        static Config *getInstance();
        ~Config();
    };

}


#endif //TLBS_CONFIG_H
