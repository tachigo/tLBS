//
// Created by liuliwu on 2020-05-28.
//

#ifndef TLBS_CONFIG_H
#define TLBS_CONFIG_H

#ifdef __APPLE__
#include <AvailabilityMacros.h>
#endif

#ifdef __linux__
#include <linux/version.h>
#include <features.h>
#endif

#ifdef __linux__
#define HAVE_EPOLL 1
#endif

#if (defined(__APPLE__) && defined(MAC_OS_X_VERSION_10_6)) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined (__NetBSD__)
#define HAVE_KQUEUE 1
#endif

//#ifdef __sun
//#include <sys/feature_tests.h>
//#ifdef _DTRACE_VERSION
//#define HAVE_EVPORT 1
//#endif
//#endif

#include <gflags/gflags.h>
#include <string>

// config file
DECLARE_string(config_file); // 配置文件的路径
DECLARE_string(bin_root); // 可执行文件的根路径

// server config
//DECLARE_string(pid_file); // PID进程锁文件
DECLARE_bool(daemonize); // 是否以守护进程启动
DECLARE_int32(server_hz); // server时间事件频率 每秒多少次

// net tcp
DECLARE_string(tcp_host); // tcp主机ip
DECLARE_string(tcp_port); // tcp端口号
DECLARE_int32(tcp_backlog); // tcp连接队列的长度
DECLARE_int32(tcp_keepalive); // tcp连接保持存活时长

// connection
DECLARE_int32(max_connections); // 最大同时产生的客户端连接数
DECLARE_int32(threads_connection); // 线程处理客户端

// db
DECLARE_int32(db_num); // 数据库的数量
DECLARE_string(db_root); // 数据库数据文件存放的根路径


// cluster
DECLARE_string(cluster_nodes); // cluster节点的链接字符串 eg. 127.0.0.1:8899;127.0.0.1:8888

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
