//
// Created by 刘立悟 on 2020/5/18.
//

#ifndef TLBS_CLIENT_H
#define TLBS_CLIENT_H

#include "db.h"
#include "connection.h"
#include <map>
#include <vector>

// 最小保留的文件描述符数
#define MIN_REVERSED_FDS 32
#define CLIENT_FLAGS_PENDING_CLOSE (1<<0)

namespace tLBS {

    typedef enum {
        CLIENT_FORMAT_LEGACY = 0,
        CLIENT_FORMAT_JSON
    } ClientFormat;

    class Client {
    private:
        uint64_t id; // 客户端id
//        Db *db; // 客户端连接的库
//        time_t ctime; // 客户端创建时间
        uint64_t flags; // 客户端标记
        Connection *conn; // 一个连接对象
        ClientFormat format; // 返回格式 legacy 历史格式, json json格式
        static _Atomic uint64_t nextClientId;
        static std::map<uint64_t, Client *> clients;
        std::string query;
        std::vector<std::string> args;
        static void parseCommandLine(const char *line, std::vector<std::string> *argv);
        int processCommand();
    public:
        Client(Connection *conn, int flags);
        static Client *getClient(uint64_t clientId);
        static void link(Client *client);
        static void free(Client *client);
        ~Client();
        uint64_t getId();
//        Db *getDb();
//        time_t getCreateTime();
        uint64_t getFlags();
        void setFlags(uint64_t flags);
        void pendingClose();
        Connection *getConnection();

        std::vector<std::string> getArgs();
        std::string arg(int i);

        ClientFormat getFormat();
        void setFormat(ClientFormat format);

        int success();
        int success(const char *msg);
        int fail(int error, const char *msg);
        int fail(const char *fmt, ...);

        void readFromConnection();
        int processCommandAndReset();

        static std::map<uint64_t, Client *> getClients();
        static void adjustMaxClients();
        static void connReadHandler(Connection *data);

        static int cron(long long id, void *data);

        // command
        static int formatSelect(Client *client);
    };
}

#endif //TLBS_CLIENT_H
