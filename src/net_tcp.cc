//
// Created by liuliwu on 2020-05-28.
//

#include "common.h"
#include "net_tcp.h"
#include "config.h"
#include "log.h"
#include "connection.h"
#include "client.h"

#include <sys/socket.h>
#include <sys/file.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>


using namespace tLBS;

DEFINE_string(tcp_port, "8899", "tcp端口号");
DEFINE_int32(tcp_backlog, 511, "tcp建立连接的队列长度");
DEFINE_int32(tcp_keepalive, 300, "连接保持存活时长");


NetTcp::NetTcp() {
    this->backlog = FLAGS_tcp_backlog;
    this->bindAddrCount = 0;
    this->tcpFdCount = 0;
}

int NetTcp::v4server(char *bindAddr) {
    info("绑定IPv4的一个网络监听");
    return this->server(bindAddr, AF_INET);
}

int NetTcp::v6server(char *bindAddr) {
    info("绑定IPv6的一个网络监听");
    return this->server(bindAddr, AF_INET6);
}

int NetTcp::server(char *bindAddr, int af) {
    int fd = -1;
    // 使用配置的tcp端口号
    std::string service = FLAGS_tcp_port;
    struct addrinfo hints, *serviceInfo, *p;
    // hints初始化为0
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = af;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int err;

    // 将主机名和服务名映射到一个地址
    if ((err = getaddrinfo(bindAddr, service.c_str(), &hints, &serviceInfo))) {
        error(gai_strerror(err));
        return NET_ERR;
    }

    for (p = serviceInfo; p != nullptr; p = p->ai_next) {
        if ((fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            continue;
        }
        if (af == AF_INET6 && this->v6Only(fd) == NET_ERR) {
            goto error;
        }
        if (this->setReuseAddr(fd) == NET_ERR) {
            goto error;
        }
        if (this->listen(fd, p->ai_addr, p->ai_addrlen) == NET_ERR) {
            fd = NET_ERR;
        }
        goto end;
    }
    if (p == nullptr) {
        error("无法绑定socket: ") <<  errno;
        goto error;
    }

error:
    if (fd != -1) {
        close(fd);
    }
    fd = NET_ERR;
end:
    freeaddrinfo(serviceInfo);
    return fd;
}

int NetTcp::listen(int fd, struct sockaddr *sa, socklen_t len) {
    if (bind(fd, sa, len) == -1) {
        error("bind: ") << strerror(errno);
        close(fd);
        return NET_ERR;
    }
    if (::listen(fd, this->backlog) == -1) {
        error("listen: ") << strerror(errno);
        close(fd);
        return NET_ERR;
    }
    return NET_OK;
}

int NetTcp::setReuseAddr(int fd) {
    int yes = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        error("setsocketopt SO_REUSEADDR: ") << strerror(errno);
        return NET_ERR;
    }
    return NET_OK;
}

int NetTcp::v6Only(int fd) {
    int yes = 1;
    if (setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &yes, sizeof(yes)) == -1) {
        error("setsocketopt IPPROTO_IPV6, IPV6_V6ONLY: ") << strerror(errno);
        close(fd);
        return NET_ERR;
    }
    return NET_OK;
}

int NetTcp::listenPort() {
    info("建立tcp网络监听");
    int j;
    if (this->bindAddrCount == 0) {
        this->bindAddr[0] = nullptr;
    }
    for (j = 0; j < this->bindAddrCount || j == 0; j++) {
        if (this->bindAddr[j] == nullptr) {
            int unsupported = 0;
            this->tcpFd[this->tcpFdCount] = this->v6server(this->bindAddr[j]);
            if (this->tcpFd[this->tcpFdCount] != NET_ERR) {
                NetTcp::setNonBlock(this->tcpFd[this->tcpFdCount]);
                this->tcpFdCount++;
            }
            else if (errno == EAFNOSUPPORT) {
                unsupported++;
                warning("不支持监听 IPv6");
            }
            if (this->tcpFdCount == 1 || unsupported) {
                this->tcpFd[this->tcpFdCount] = this->v4server(this->bindAddr[j]);
                if (this->tcpFd[this->tcpFdCount] != NET_ERR) {
                    NetTcp::setNonBlock(this->tcpFd[this->tcpFdCount]);
                    this->tcpFdCount++;
                }
                else if (errno == EAFNOSUPPORT) {
                    unsupported++;
                    warning("不支持监听 IPv4");
                }
            }
            if (this->tcpFdCount + unsupported == 2) {
                break;
            }
        }
        else if (strchr(this->bindAddr[j], ':')) {
            this->tcpFd[this->tcpFdCount] = this->v6server(this->bindAddr[j]);
        }
        else {
            this->tcpFd[this->tcpFdCount] = this->v4server(this->bindAddr[j]);
        }
        if (this->tcpFd[this->tcpFdCount] == NET_ERR) {
            warning("无法创建监听TCP套接字的服务器 ")
                << (this->bindAddr[j] ? this->bindAddr[j] : "*")
                << (":") << FLAGS_tcp_port;
            if (errno == ENOPROTOOPT     || errno == EPROTONOSUPPORT ||
                errno == ESOCKTNOSUPPORT || errno == EPFNOSUPPORT ||
                errno == EAFNOSUPPORT    || errno == EADDRNOTAVAIL)
                continue;
            return C_ERR;
        }
        NetTcp::setNonBlock(this->tcpFd[this->tcpFdCount]);
        this->tcpFdCount++;
    }
    return C_OK;
}

int NetTcp::setNonBlock(int fd) {
    int flags;

    if ((flags = fcntl(fd, F_GETFL)) == -1) {
        error("fcntl(F_GETFL): ") << strerror(errno);
        return NET_ERR;
    }

    flags |= O_NONBLOCK;

    if (fcntl(fd, F_SETFL, flags) == -1) {
        error("fcntl(F_SETFL, O_NONBLOCK): ") << strerror(errno);
        return NET_ERR;
    }
    return NET_OK;
}

int NetTcp::setBlock(int fd) {
    int flags;

    if ((flags = fcntl(fd, F_GETFL)) == -1) {
        error("fcntl(F_GETFL): ") << strerror(errno);
        return NET_ERR;
    }

    flags &= ~O_NONBLOCK;

    if (fcntl(fd, F_SETFL, flags) == -1) {
        error("fcntl(F_SETFL, O_NONBLOCK): ") << strerror(errno);
        return NET_ERR;
    }
    return NET_OK;
}

int* NetTcp::getTcpFd() {
    return this->tcpFd;
}

int NetTcp::getTcpFdCount() {
    return this->tcpFdCount;
}

int NetTcp::setNoDelay(int fd, int val) {
    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val)) == -1) {
        error("setsockopt TCP_NODELAY: ") << strerror(errno);
        return NET_ERR;
    }
    return NET_OK;
}

int NetTcp::setKeepalive(int fd, int interval) {
    int val = 1;

    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val)) == -1) {
        error("setsockopt SO_KEEPALIVE: ") << strerror(errno);
        return NET_ERR;
    }

#ifdef __linux__
    /* Default settings are more or less garbage, with the keepalive time
     * set to 7200 by default on Linux. Modify settings to make the feature
     * actually useful. */

    /* Send first probe after interval. */
    val = interval;
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &val, sizeof(val)) < 0) {
        error("setsockopt TCP_KEEPIDLE: ") << strerror(errno);
        return NET_ERR;
    }

    /* Send next probes after the specified interval. Note that we set the
     * delay as interval / 3, as we send three probes before detecting
     * an error (see the next setsockopt call). */
    val = interval/3;
    if (val == 0) val = 1;
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &val, sizeof(val)) < 0) {
        error("setsockopt TCP_KEEPINTVL: ") << strerror(errno);
        return NET_ERR;
    }

    /* Consider the socket in error state after three we send three ACK
     * probes without getting a reply. */
    val = 3;
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &val, sizeof(val)) < 0) {
        error("setsockopt TCP_KEEPCNT: ") << strerror(errno);
        return NET_ERR;
    }
#else
    ((void) interval); /* Avoid unused var warning for non Linux systems. */
#endif

    return NET_OK;
}

int NetTcp::genericAccept(int s, struct sockaddr *sa, socklen_t *len) {
    int fd;
    while (true) {
        fd = ::accept(s, sa, len);
        if (fd == -1) {
            if (errno == EINTR) {
                // 中断
                continue;
            }
            else {
                error("accept: ") << strerror(errno);
                return NET_ERR;
            }
        }
        break;
    }
    return fd;
}

void NetTcp::acceptHandler(EventLoop *el, int fd, int flags, void *data) {
    UNUSED(el);
    UNUSED(flags);
    UNUSED(data);

    int maxAcceptsPerCall = 1000;
    char connIp[INET6_ADDRSTRLEN];
    int connPort, connFd;
    while (maxAcceptsPerCall--) {
        // 接收一个套接字
        connFd = NetTcp::accept(fd, connIp, sizeof(connIp), &connPort);
        if (connFd == NET_ERR) {
            if (errno != EWOULDBLOCK) {
                error("接受客户端连接失败");
            }
            return;
        }
        // 这里可以使用多线程方式来处理
        warning("接受的连接fd#") << connFd << " " << connIp << ":" << connPort;
        // 创建一个连接对象
        auto *conn = new Connection(connFd);
        if (Client::getClients().size() >= FLAGS_max_clients) {
            // 客户端连接数量超了
            const char *err = "-ERR max number of clients reached\r\n";
            conn->write(err, strlen(err));
            conn->close(el);
            return;
        }
        // 创建一个客户端连接对象
        auto *client = new Client(conn, flags);
        NetTcp::setNonBlock(connFd);
        NetTcp::setNoDelay(connFd, 1);
        if (FLAGS_tcp_keepalive > 0) {
            NetTcp::setKeepalive(connFd, FLAGS_tcp_keepalive);
        }
        // 将新生成的存储到clients列表中
        Client::linkClient(client);

        const char *err = "-OK hello world!你好啊!~\r\n";
        conn->write(err, strlen(err));
    }
}

int NetTcp::accept(int s, char *ip, size_t ipLen, int *port) {
    int fd;
    struct sockaddr_storage sa;
    socklen_t  saLen = sizeof(sa);
    if ((fd = NetTcp::genericAccept(s, (struct sockaddr *) &sa, &saLen)) == -1) {
        return NET_ERR;
    }
    if (sa.ss_family == AF_INET) {
        struct sockaddr_in *s = (struct sockaddr_in *) &sa;
        if (ip) {
            inet_ntop(AF_INET, (void *) &(s->sin_addr), ip, ipLen);
        }
        if (port) {
            *port = ntohs(s->sin_port);
        }
    }
    else {
        struct sockaddr_in6 *s = (struct sockaddr_in6 *) &sa;
        if (ip) {
            inet_ntop(AF_INET, (void *) &(s->sin6_addr), ip, ipLen);
        }
        if (port) {
            *port = ntohs(s->sin6_port);
        }
    }

    return fd;
}

NetTcp::~NetTcp() {

}