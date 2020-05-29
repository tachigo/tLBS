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
        Object *name; // 当前客户端的名称
//        time_t ctime; // 客户端创建时间
        uint64_t flags; // 客户端标记
        Connection *conn; // 一个连接对象

        static std::map<uint64_t, Client *> clients;
        static _Atomic uint64_t nextClientId;
    public:
        Client(Connection *conn, int flags);
        ~Client();
        uint64_t getId();
//        Db *getDb();
        Object *getName();
//        time_t getCreateTime();
        uint64_t getFlags();

        static std::map<uint64_t, Client *> getClients();
        static void linkClient(Client *client);
        static void unlinkClient(Client *client);

        static void adjustMaxClients();
    };
}

#endif //TLBS_CLIENT_H
