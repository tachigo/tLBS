//
// Created by liuliwu on 2020-05-28.
//

#ifndef TLBS_NET_TCP_H
#define TLBS_NET_TCP_H

#define NET_OK 0
#define NET_ERR 1

#include <netdb.h>

namespace tLBS {
    class NetTcp {
    private:

        int backlog;

        char *bindAddr[16]; // 绑定的地址列表
        int bindAddrCount; // 绑定的地址数量

        int tcpFd[16]; // tcp套接字文件描述符列表
        int tcpFdCount; // tcp套接字文件描述符数量
        int v6Only(int fd);
        int setReuseAddr(int fd);
        int listen(int fd, struct sockaddr *sa, socklen_t len);
        int server(char *bindAddr, int af);
    public:
        NetTcp();
        ~NetTcp();
        int v4server(char *bindAddr); // 创建ipv4的服务器
        int v6server(char *bindAddr); // 创建ipv6的服务器
        int setBlock(int fd);
        int setNonBlock(int fd);
        int listenPort();
        int getTcpFdCount();
        int *getTcpFd();
    };
}

#endif //TLBS_NET_TCP_H
