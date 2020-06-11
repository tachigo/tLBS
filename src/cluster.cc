//
// Created by liuliwu on 2020-06-10.
//

#include "cluster.h"
#include "config.h"
#include "connection.h"
#include "log.h"
#include "net_tcp.h"

#include <regex>
#include <sstream>


DEFINE_string(cluster_nodes, "", "cluster节点的链接字符串 eg. 127.0.0.1:8899;127.0.0.1:8888");

using namespace tLBS;

std::map<std::string, ClusterNode *> Cluster::nodes;

int Cluster::unEstablishedNodeCount = 0;

void Cluster::init() {
    if (FLAGS_cluster_nodes.size() > 0) {
        std::regex reg(";");
        std::vector<std::string> v(
                std::sregex_token_iterator(
                        FLAGS_cluster_nodes.begin(), FLAGS_cluster_nodes.end(), reg, -1
                ),
                std::sregex_token_iterator());
        for (int i = 0; i < v.size(); i++) {
            std::string clusterNodeUrl = v[i];
            addNode(clusterNodeUrl);
        }
        unEstablishedNodeCount = v.size();
    }
    else {
        warning("没有集群");
    }
    tryConnect();
}


void Cluster::addNode(std::string nodeUrl, tLBS::Connection *conn) {
    std::regex reg(":");
    std::vector<std::string> v(
            std::sregex_token_iterator(
                    nodeUrl.begin(), nodeUrl.end(), reg, -1
            ),
            std::sregex_token_iterator());
    std::string ip = v[0];
    std::stringstream is(v[1]);
    int port;
    is >> port;
    auto clusterNode = new ClusterNode(ip, port);
    nodes[nodeUrl] = clusterNode;
    clusterNode->setConnection(conn);
    clusterNode->setEstablished(true);
    char buf[100];
    snprintf(buf, sizeof(buf) - 1, "cluster[%s:%d][fd:%d]", clusterNode->getIp().c_str(), clusterNode->getPort(), conn->getFd());
    clusterNode->setInfo(buf);
    conn->setInfo(clusterNode->getInfo());
    warning("join cluster node: ") << clusterNode->getInfo();
}


void Cluster::addNode(std::string nodeUrl) {
    std::regex reg(":");
    std::vector<std::string> v(
            std::sregex_token_iterator(
                    nodeUrl.begin(), nodeUrl.end(), reg, -1
            ),
            std::sregex_token_iterator());
    std::string ip = v[0];
    std::stringstream is(v[1]);
    int port;
    is >> port;
    auto clusterNode = new ClusterNode(ip, port);
    nodes[nodeUrl] = clusterNode;
    info("config cluster node: ") << clusterNode->getInfo() << " going to join...";
}

void Cluster::connConnectHandler(tLBS::Connection *conn) {
    auto node = (ClusterNode *)conn->getContainer();
    if (conn->getState() != ConnectionState::CONN_STATE_CONNECTED) {
//        warning(conn->getInfo()) << " 建立连接失败: " << strerror(conn->getLastErrno()) << "(" << conn->getLastErrno() << ")";
        node->closeConnection();
        decrUnEstablishedNodeCount();
        return;
    }

    warning(node->getInfo()) << " 建立连接成功";
    decrUnEstablishedNodeCount();
    conn->setInfo(node->getInfo());
    node->setConnection(conn);
    node->setEstablished(true);
    // 设置读的handler
    conn->setReadHandler(connReadClusterJoinHandler);
    // 发送join
    joinCluster(conn);
}


void Cluster::connReadClusterJoinHandler(tLBS::Connection *conn) {
    if (conn->getState() == ConnectionState::CONN_STATE_CLOSED) {
        return;
    }
    auto node = (ClusterNode *)conn->getContainer();
    int segLen = (1024); // 32M
    int totalRead = 0;
    std::string qb;
    char buf[segLen];
    while (true) {
        // 每次读出一部分
        memset(buf, 0, segLen);
        int nRead = conn->read(buf, segLen);
        if (nRead == -1) {
            if (conn->getLastErrno() > 0) {
//                info(conn->getInfo()) << "已读取(" << totalRead << "): " << qb;
                error(conn->getInfo()) << "读取数据错误: " << strerror(conn->getLastErrno()) << "(" << conn->getLastErrno() << ")";
                node->closeConnection();
                return;
            }
            else {
//                error(conn->getInfo()) << "读取数据错误: " << strerror(conn->getLastErrno()) << "(" << conn->getLastErrno() << ")";
                break;
            }
        }
        else {
            if (nRead == 0) {
                if (strlen(buf) > 0) {
//                    info(conn->getInfo()) << "被关闭？但是还是有数据: " << buf;
                    buf[segLen] = '\0'; // 确保最后一个字符是\0
                    // 有新的内容 追加进去
                    totalRead += strlen(buf);
                    qb += buf;
                    break;
                }
                else {
                    info(conn->getInfo()) << "客户端关闭连接,节点准备断开";
                    node->closeConnection();
                    return;
                }
            }
            else if (strlen(buf) > 0) {
                buf[segLen] = '\0'; // 确保最后一个字符是\0
                // 有新的内容 追加进去
                totalRead += strlen(buf);
                qb += std::string(buf);
            }
        }
    }
    if (totalRead == 0) {
        warning(conn->getInfo()) << "读取数据长度为0, " << conn->getInfo() << "准备关闭";
        node->closeConnection();
        return;
    } else {
        info(conn->getInfo()) << "读取数据长度为: " << totalRead << std::endl
            << qb;
    }
}

// 读取连接的其他节点发送过来的数据
void Cluster::connReadHandler(tLBS::Connection *conn) {
    if (conn->getState() == ConnectionState::CONN_STATE_CLOSED) {
        return;
    }
    auto node = (ClusterNode *)conn->getContainer();
    int segLen = (1024); // 32M
    int totalRead = 0;
    std::string qb;
    char buf[segLen];
    while (true) {
        // 每次读出一部分
        memset(buf, 0, segLen);
        int nRead = conn->read(buf, segLen);
        if (nRead == -1) {
            if (conn->getLastErrno() > 0) {
//                info(conn->getInfo()) << "已读取(" << totalRead << "): " << qb;
                error(conn->getInfo()) << "读取数据错误: " << strerror(conn->getLastErrno()) << "(" << conn->getLastErrno() << ")";
                node->closeConnection();
                return;
            }
            else {
//                error(conn->getInfo()) << "读取数据错误: " << strerror(conn->getLastErrno()) << "(" << conn->getLastErrno() << ")";
                break;
            }
        }
        else {
            if (nRead == 0) {
                if (strlen(buf) > 0) {
//                    info(conn->getInfo()) << "被关闭？但是还是有数据: " << buf;
                    buf[segLen] = '\0'; // 确保最后一个字符是\0
                    // 有新的内容 追加进去
                    totalRead += strlen(buf);
                    qb += buf;
                    break;
                }
                else {
                    info(conn->getInfo()) << "客户端关闭连接,节点准备断开";
                    node->closeConnection();
                    return;
                }
            }
            else if (strlen(buf) > 0) {
                buf[segLen] = '\0'; // 确保最后一个字符是\0
                // 有新的内容 追加进去
                totalRead += strlen(buf);
                qb += std::string(buf);
            }
        }
    }
    if (totalRead == 0) {
        warning(conn->getInfo()) << "读取数据长度为0, " << conn->getInfo() << "准备关闭";
        node->closeConnection();
        return;
    } else {
        info(conn->getInfo()) << "读取数据长度为: " << totalRead << std::endl
            << qb;
    }
}

void Cluster::connWriteHandler(tLBS::Connection *conn) {
    conn->setWriteHandler(nullptr);
}

void Cluster::tryConnect() {
    // 尝试建立连接
    EventLoop *el = EventLoop::getInstance();
    if (unEstablishedNodeCount > 0) {
//        info("尝试建立尚未建立cluster节点的连接: (") << unEstablishedNodeCount << ")";
        NetTcp *net = NetTcp::getInstance();
        for (auto mapIter = nodes.begin(); mapIter != nodes.end(); mapIter++) {
            auto node = mapIter->second;
            if (!node->getEstablished()) {
                // 没有建立连接
                int cfd = net->connect(node->getIp().c_str(), node->getPort(), nullptr, NET_CONNECT_NONBLOCK | NET_CONNECT_BE_BINDING);
                if (cfd > 0) {
                    auto conn = new Connection(cfd, ConnectionState::CONN_STATE_CONNECTING);

                    char buf[100];
                    snprintf(buf, sizeof(buf) - 1, "cluster[%s:%d][fd:%d]", node->getIp().c_str(), node->getPort(), conn->getFd());
                    node->setInfo(buf);

                    conn->setInfo(node->getInfo());
                    conn->setConnHandler(connConnectHandler);
                    conn->setContainer((void *)node);
                    node->setConnection(conn);
                    el->addFileEvent(cfd, EL_WRITABLE, Connection::eventHandler, conn);
                }
            }
        }
    }
//    for (auto mapIter = nodes.begin(); mapIter != nodes.end(); mapIter++) {
//        auto node = mapIter->second;
//        if (node->getEstablished()) {
//            auto conn = node->getConnection();
//            conn->setWriteHandler(connPingHandler);
//            el->addFileEvent(conn->getFd(), EL_WRITABLE, Connection::eventHandler, conn);
//        }
//    }
}


//void Cluster::connPingHandler(tLBS::Connection *conn) {
//    conn->success("clusterping");
//    conn->setWriteHandler(nullptr);
//}

void Cluster::free() {
    info("销毁所有cluster节点");
    std::vector<ClusterNode *> nv;
    for (auto mapIter = nodes.begin(); mapIter != nodes.end(); mapIter++) {
        nv.push_back(mapIter->second);
    }
    for (int i = 0; i < nv.size(); i++) {
        auto node = nv[i];
        nodes.erase(node->getInfo());
        delete node;
    }
}


void Cluster::incrUnEstablishedNodeCount() {
    unEstablishedNodeCount += 1;
//    info("当前未建立cluster连接的节点有: ") << unEstablishedNodeCount;
}

void Cluster::decrUnEstablishedNodeCount() {
    unEstablishedNodeCount -= 1;
//    info("当前未建立cluster连接的节点有: ") << unEstablishedNodeCount;
}



ClusterNode::ClusterNode(std::string ip, int port) {
    this->ip = ip;
    this->port = port;
    this->conn = nullptr;
    this->established = false;
    char buf[100];
    snprintf(buf, sizeof(buf) - 1, "cluster[%s:%d]", ip.c_str(), port);
    this->info = buf;
}

ClusterNode::~ClusterNode() {
    closeConnection();
}

void ClusterNode::setConnection(tLBS::Connection *conn) {
    this->conn = conn;
}

Connection* ClusterNode::getConnection() {
    return this->conn;
}

std::string ClusterNode::getInfo() {
    return this->info;
}

void ClusterNode::setInfo(std::string info) {
    this->info = info;
}

std::string ClusterNode::getIp() {
    return this->ip;
}

int ClusterNode::getPort() {
    return this->port;
}

void ClusterNode::setEstablished(bool established) {
    this->established = established;
}

bool ClusterNode::getEstablished() {
    return this->established;
}


void ClusterNode::closeConnection() {
    if (this->conn != nullptr) {
        this->conn->pendingClose();
        this->conn = nullptr;
        Cluster::incrUnEstablishedNodeCount();
    }
    this->established = false;
}

// send
int Cluster::joinCluster(tLBS::Connection *conn) {
    auto node = (ClusterNode *)conn->getContainer();
    char msg[1024];
    snprintf(msg, sizeof(msg) - 1, "clusterjoin %s:%s", FLAGS_tcp_host.c_str(), FLAGS_tcp_port.c_str());
    return conn->success(msg);
}

// recv
int Cluster::execClusterJoin(tLBS::Connection *conn, std::vector<std::string> args) {
    if (args.size() != 2) {
        return conn->fail(Json::createErrorJsonObj(ERRNO_EXEC_SYNTAX_ERR, ERROR_EXEC_SYNTAX_ERR));
    }
    std::string addr = args[1];
    addNode(addr, conn);
    return conn->success(Json::createSuccessStringJsonObj("clusterjoin"));
}

// recv
int Cluster::execClusterPing(tLBS::Connection *conn, std::vector<std::string> args) {
    UNUSED(args);
    return conn->success(Json::createSuccessStringJsonObj("clusterpong"));
}


// recv
int Cluster::execClusterNodes(Connection *conn, std::vector<std::string> args) {
    UNUSED(args);

    return conn->success(Json::createSuccessStringJsonObj("OK"));
}