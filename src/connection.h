//
// Created by liuliwu on 2020-05-29.
//

#ifndef TLBS_CONNECTION_H
#define TLBS_CONNECTION_H

#include <string>

namespace tLBS {

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
        int fd; // 连接的文件描述符
        ConnectionState state;
        short int flags;
        short int refs;
        int lastErrno;
        void *data;
    public:

        Connection(); // 创建一个空的tcp连接对象
        explicit Connection(int fd); // 通过文件描述符来创建tcp连接对象
        ~Connection();

        std::string getInfo();
        void incrRefs();
        void decrRefs();
        int getRefs();

//        void connHandler();
//        void writeHandler();
//        void readHandler();
    };
}

#endif //TLBS_CONNECTION_H
