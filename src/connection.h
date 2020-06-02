//
// Created by liuliwu on 2020-05-29.
//

#ifndef TLBS_CONNECTION_H
#define TLBS_CONNECTION_H

#include <string>
#include <map>
#include "el.h"

#define CONN_FLAG_CLOSE_SCHEDULED   (1<<0)      /* Closed scheduled by a handler */
#define CONN_FLAG_WRITE_BARRIER     (1<<1)      /* Write barrier requested */

namespace tLBS {

    class Connection;

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
        void *data;

        ConnectionFallback connectHandler;
        ConnectionFallback readHandler;
        ConnectionFallback writeHandler;

    public:
        explicit Connection(int fd); // 通过文件描述符来创建tcp连接对象
        static void free(Connection *conn);
        ~Connection();

        std::string getInfo();
        void incrRefs();
        void decrRefs();
        int getRefs();
        uint64_t getId();
        int getFd();
        void setData(void *data);
        void *getData();
        int getLastErrno();
        int getFlags();
        void setFlags(int flags);
        ConnectionState getState();
        void setState(ConnectionState state);
        void scheduleClose();

        int write(const void *data, size_t dataLen); // 向连接写数据
        int read(void *buf, size_t bufLen); // 从连接读数据
        void close(); // 关闭连接

        static void eventHandler(int fd, int flags, void *data);

        int setConnectHandler(ConnectionFallback handler);
        ConnectionFallback getConnectHandler();
        int setReadHandler(ConnectionFallback handler);
        ConnectionFallback getReadHandler();
        int setWriteHandler(ConnectionFallback handler);
        ConnectionFallback getWriteHandler();

        int invokeHandler(ConnectionFallback handler);

//        void connHandler();
//        void writeHandler();
//        void readHandler();
    };

}

#endif //TLBS_CONNECTION_H
