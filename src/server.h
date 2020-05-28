//
// Created by 刘立悟 on 2020/5/18.
//

#ifndef TLBS_SERVER_H
#define TLBS_SERVER_H

#include <string>
#include <vector>

namespace tLBS {
    class Server {
    private:
        pid_t pid; // 进程号
        int shutdownAsap; // 服务器关闭标识
        int archBits; // 操作系统是多少位的

        void createPidFile();
        void daemonize();
    public:
        Server();
        ~Server();
        pid_t getPid();
        int getShutdownAsap();
        int getArchBits();
        void init();
    };
}

#endif //TLBS_SERVER_H
