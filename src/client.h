//
// Created by 刘立悟 on 2020/5/18.
//

#ifndef TLBS_CLIENT_H
#define TLBS_CLIENT_H

#include "db.h"
#include "connection.h"
#include <map>

// 最小保留的文件描述符数
#define MIN_REVERSED_FDS 32

namespace tLBS {

    class Client {
    private:
        uint64_t id; // 客户端id
//        Db *db; // 客户端连接的库
//        Object *name; // 当前客户端的名称
//        time_t ctime; // 客户端创建时间
        uint64_t flags; // 客户端标记
        Connection *conn; // 一个连接对象
        const char *queryBuf;
        static _Atomic uint64_t nextClientId;
        static std::map<uint64_t, Client *> clients;
    public:
        Client(Connection *conn, int flags);
        static Client *create(Connection *conn, int flags);
        static Client *getClient(uint64_t clientId);
        static void link(Client *client);
        static void free(Client *client);
        ~Client();
        uint64_t getId();
//        Db *getDb();
//        Object *getName();
//        time_t getCreateTime();
        uint64_t getFlags();
        Connection *getConnection();

        void readFromConnection();

        static std::map<uint64_t, Client *> getClients();
        static void adjustMaxClients();
        static void connReadHandler(Connection *data);
    };
}

#endif //TLBS_CLIENT_H
