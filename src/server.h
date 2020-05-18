//
// Created by 刘立悟 on 2020/5/18.
//

#ifndef TLBS_SERVER_H
#define TLBS_SERVER_H

#include <string>
#include <vector>
#include "db.h"

namespace tLBS {
    class Server {
    private:
        pid_t pid; // 进程号
        std::string pidFilename; // 进程文件
        std::string configFilename; // 配置文件
        std::string logFilename; // 日志文件
        std::string executable; // 可执行文件地址
        int hz; // 赫兹，server轮询频率
        int dbNum; // 数据库的数量
        int daemonize; // 是否是守护进程模式启动
        int shutdownAsap; // 服务器关闭标识
        int archBits; // 操作系统是多少位的

        std::vector<Db *> dbs; // 数据库列表

        // 网络连接
        int tcpPort; // tcp端口

    public:
        Server();
        ~Server();
        pid_t getPid();
        std::string getPidFilename();
        std::string getConfigFilename();
        std::string getLogFilename();
        std::string getExecutable();
        int getHz();
        int getDbNum();
        int isDaemonize();
        int getShutdownAsap();
        int getArchBits();


        // 网络
        int getTcpPort();
    };
}

#endif //TLBS_SERVER_H
