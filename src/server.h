//
// Created by 刘立悟 on 2020/5/18.
//

#ifndef TLBS_SERVER_H
#define TLBS_SERVER_H

#include <string>
#include <vector>

// server关闭的flags
#define SERVER_SHUTDOWN_NO_FLAGS 0

namespace tLBS {
    class Server {
    private:
        static Server *instance;
        pid_t pid; // 进程号
        int shutdownAsap; // 服务器关闭标识
        int archBits; // 操作系统是多少位的
        int cronHz; // server时间事件的频率
        bool daemonized; // 是否守护进程化
        std::string binRoot; // server进程的根路径
        std::string executable; // 可执行文件

        _Atomic time_t unixTime;
        long long msTime; // 毫秒
        long long usTime; // 微秒

        void daemonize();
        Server();
    public:
        static Server * getInstance();
        ~Server();
        std::string getExecutable();
        void setExecutable(std::string executable);
        std::string getBinRoot();
        void createPidFile();
        void deletePidFile();
        pid_t getPid();
        int getShutdownAsap();
        void setShutdownAsap(int asap);
        int getArchBits();
        int getCronHz();
        bool isDaemonized();
        time_t getUnixTime();
        long long getUsTime();
        void updateCachedTime();
        void init();
        static void shutdown(int sig);
        static int prepareShutdown(int flags);
        static int cron(long long id, void *data);
        static void beforeSleep();
        static void free();
    };
}

#endif //TLBS_SERVER_H
