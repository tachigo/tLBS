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
        std::string executable; // 可执行文件路径
        int shutdownAsap; // 服务器关闭标识
        int archBits; // 操作系统是多少位的

        std::vector<Db *> dbs; // 数据库列表

    public:
        Server();
        ~Server();
        pid_t getPid();
        std::string getPidFilename();
        std::string getConfigFilename();
        std::string getLogFilename();
        std::string getExecutable();
        int getShutdownAsap();
        int getArchBits();
    };
}

#endif //TLBS_SERVER_H
