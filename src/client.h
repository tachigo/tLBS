//
// Created by 刘立悟 on 2020/5/18.
//

#ifndef TLBS_CLIENT_H
#define TLBS_CLIENT_H

#include "connection.h"
#include "json.h"
#include <map>
#include <vector>

// 最小保留的文件描述符数
#define MIN_REVERSED_FDS 32
#define CLIENT_FLAGS_PENDING_CLOSE (1<<0)
#define CLIENT_FLAGS_CLOSE_AFTER_REPLY (1<<1)

namespace tLBS {

    class Db;

    class Client {
    private:
        uint64_t id; // 客户端id
        std::string info;
        Db *db; // 客户端连接的库
//        time_t ctime; // 客户端创建时间
        uint64_t flags; // 客户端标记
        Connection *conn; // 一个连接对象
        static _Atomic uint64_t nextClientId;
        static std::map<uint64_t, Client *> clients;
        std::string query;
        std::vector<std::string> args;
        std::string response;
        int sent; // 发送数据的数量
        bool http;
        void parseQueryBuff(const char *line, std::vector<std::string> *argv);
    public:
        Client(Connection *conn, int flags);
        static Client *getClient(uint64_t clientId);
        static void link(Client *client);
        static void free(Client *client);
        ~Client();
        uint64_t getId();
        Db *getDb();
        void setDb(Db *db);
//        time_t getCreateTime();
        uint64_t getFlags();
        void setFlags(uint64_t flags);
        void pendingClose();
        Connection *getConnection();
        std::string getInfo();
        void setHttp(bool http);
        bool isHttp();
        void setResponse(std::string response);

        int getSent();
        void setSent(int sent);

        std::vector<std::string> getArgs();
        std::string arg(int i);

        int success();
        int success(const char *msg);
        int success(Json *json);
        int fail(int error, const char *msg);
        int fail(const char *fmt, ...);
        int fail(Json *json);

        void readFromConnection();
        void writeToConnection();

        static std::map<uint64_t, Client *> getClients();
        static void adjustMaxClients();
        static void connReadHandler(Connection *data);
        static void connWriteHandler(Connection *data);

        static int cron(long long id, void *data);


        // cmd
        static int cmdQuit(Client *client);

    };
}

#endif //TLBS_CLIENT_H
