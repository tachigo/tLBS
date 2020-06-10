//
// Created by liuliwu on 2020-05-28.
//

#ifndef TLBS_NET_TCP_H
#define TLBS_NET_TCP_H

#define NET_OK 0
#define NET_ERR -1

#define NET_CONNECT_NONE 0
#define NET_CONNECT_NONBLOCK 1
#define NET_CONNECT_BE_BINDING 2 /* Best effort binding. */

#include <netdb.h>
#include "el.h"
#include "connection.h"

namespace tLBS {

    class Connection;

    class NetTcp {
    private:
        static NetTcp *instance;
        int backlog;

        char *bindAddr[16]; // 绑定的地址列表
        int bindAddrCount; // 绑定的地址数量

        int tcpFd[16]; // tcp套接字文件描述符列表
        int tcpFdCount; // tcp套接字文件描述符数量
        int v6Only(int fd);
        int setReuseAddr(int fd);
        int listen(int fd);
        int bind(int fd, struct sockaddr *sa, socklen_t len);
        int server(char *bindAddr, int af);
        static int genericAccept(int s, struct sockaddr *sa, socklen_t *len);
        NetTcp();
    public:
        static NetTcp *getInstance();
        static void free();
        ~NetTcp();
        int v4server(char *bindAddr); // 创建ipv4的服务器
        int v6server(char *bindAddr); // 创建ipv6的服务器

        int bindAndListen();
        int getTcpFdCount();
        int *getTcpFd();
        static void acceptHandler(int fd, int flags, void *data);
        static void acceptCommonHandler(Connection *conn, int flags);
        static int accept(int fd, char *ip, size_t ipLen, int *port);
        static int setBlock(int fd);
        static int setNonBlock(int fd);
        static int setNoDelay(int fd, int val);
        static int setKeepalive(int fd, int interval);
        static int peerToString(int fd, char *ip, size_t ip_len, int *port);
        static int checkError(int fd);


        // connect
        int connect(const char *addr, int port, const char *sourceAddr, int flags);


        // threads
//        class CThreadCreateClientArgs {
//        private:
//            Connection *conn;
//            int flags;
//            std::string info;
//        public:
//            CThreadCreateClientArgs(Connection *conn, int flags);
//            std::string getInfo();
//            ~CThreadCreateClientArgs();
//            Connection *getConnection();
//            int getFlags();
//        };
//        static void *createClient(void *arg);
    };
}

#endif //TLBS_NET_TCP_H
