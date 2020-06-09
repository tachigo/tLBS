//
// Created by liuliwu on 2020-05-29.
//

#ifndef TLBS_CONNECTION_H
#define TLBS_CONNECTION_H

#include <string>
#include <map>
#include "el.h"
#include "json.h"

// 最小保留的文件描述符数
#define MIN_REVERSED_FDS 32
// 计划关闭
#define CONN_FLAGS_NONE 0
#define CONN_FLAGS_PENDING_CLOSE 1

namespace tLBS {

    class Connection;
    class Db;

    typedef void (*ConnectionFallback)(Connection *conn);

    typedef enum {
        CONN_STATE_NONE = 0,
        CONN_STATE_CONNECTING,
        CONN_STATE_ACCEPTING,
        CONN_STATE_CONNECTED,
        CONN_STATE_CLOSED,
        CONN_STATE_ERROR
    } ConnectionState;


    class Connection {
    private:
        static _Atomic uint64_t nextConnectionId;
        uint64_t id;
        std::string info;
        int fd; // 连接的文件描述符
        ConnectionState state;
        short int flags;
        short int refs;
        int lastErrno;

//        ConnectionFallback connHandler;
        ConnectionFallback readHandler;
        ConnectionFallback writeHandler;

        Db *db; // 客户端连接的库
        bool http;


    public:

        void setHttp(bool http);
        bool isHttp();

        explicit Connection(int fd); // 通过文件描述符来创建tcp连接对象
        ~Connection();

        std::string getInfo();
        void incrRefs();
        void decrRefs();
        int getRefs();
        uint64_t getId();
        int getFd();
        int getLastErrno();
        int getFlags();
        void setFlags(int flags);
        ConnectionState getState();
        void setState(ConnectionState state);

        int write(const void *data, size_t dataLen); // 向连接写数据
        int read(void *buf, size_t bufLen); // 从连接读数据
        void pendingClose();
        bool isPendingClose();
        void close(); // 关闭连接

        static void eventHandler(int fd, int flags, void *data);
        static void connReadHandler(Connection *conn);
        static void *threadProcess(void *arg);
        static void adjustMaxConnections();


        int setReadHandler(ConnectionFallback handler);
        ConnectionFallback getReadHandler();
        int setWriteHandler(ConnectionFallback handler);
        ConnectionFallback getWriteHandler();

//        int setConnHandler(ConnectionFallback handler);
//        ConnectionFallback getConnHandler();

        int invokeHandler(ConnectionFallback handler);


        int success();
        int success(const char *msg);
        int success(Json *json);
        int fail(int error, const char *msg);
        int fail(const char *fmt, ...);
        int fail(Json *json);

        Db *getDb();
        void setDb(Db *db);


        class ThreadArg {
        private:
            Connection *connection;
            std::string query;
        public:
            ThreadArg(Connection *conn, std::string query);
            ~ThreadArg() = delete;
            Connection *getConnection();
            std::string getQuery();
        };
    };

}

#endif //TLBS_CONNECTION_H
