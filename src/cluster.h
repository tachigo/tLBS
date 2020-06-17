//
// Created by liuliwu on 2020-06-10.
//

#ifndef TLBS_CLUSTER_H
#define TLBS_CLUSTER_H

#include <map>
#include <string>

#include "exec.h"

#define CLUSTER_NODE_ROLE_NONE 0
// 节点维护的连接是主动发起的
#define CLUSTER_NODE_ROLE_CONNECT (1<<0)
// 节点维护的链接是被动接收的
#define CLUSTER_NODE_ROLE_ACCEPTED (1<<1)

// 节点状态
#define CLUSTER_NODE_FLAGS_NONE 0
#define CLUSTER_NODE_FLAGS_CONNECTING (1<<0)
#define CLUSTER_NODE_FLAGS_CONNECTED (1<<1)
#define CLUSTER_NODE_FLAGS_JOINING (1<<2)
#define CLUSTER_NODE_FLAGS_JOINED (1<<3)

#define CLUSTER_NODE_FLAGS_SENDER_SYNC_ING (1<<4)
#define CLUSTER_NODE_FLAGS_SENDER_SYNC_OK (1<<5)

#define CLUSTER_NODE_FLAGS_RECEIVER_SYNC_ING (1<<6)
#define CLUSTER_NODE_FLAGS_RECEIVER_SYNC_OK (1<<7)

#define CLUSTER_NODE_FLAGS_ESTABLISH (1<<8)

namespace tLBS {

    class Connection;

    class ClusterNode {
    private:
        std::string info;
        std::string ip;
        int port;
        Connection *conn;
        int role;
        uint64_t flags;

    public:
        ClusterNode(std::string ip, int port);
        ~ClusterNode();
        std::string getInfo();
        void setInfo(std::string info);
        std::string getIp();
        int getPort();
        Connection *getConnection();
        void setConnection(Connection *conn);

        void setFlags(uint64_t flags);
        uint64_t getFlags();
        void setRole(int role);
        int getRole();

        void closeConnection();
    };

    class Cluster {
    private:
        static std::map<std::string, ClusterNode *> nodes;
    public:

        static void init();
        static void free();
        static void addNode(std::string nodeUrl);
        static void addNode(std::string nodeUrl, Connection *conn);

        static void tryReady();
        static void connConnectHandler(Connection *conn);
        static void connReadHandler(Connection *conn);
        static void connWriteHandler(Connection *conn);
        static int connRead(Connection *conn, std::string *qb);

        // join
        static int joinCluster(Connection *conn);
        static int execClusterJoin(Exec *exec, Connection *conn, std::vector<std::string> args);
        static void connReadClusterJoinHandler(Connection *conn);

        // sync start
        static int startSyncCluster(Connection *conn);
        static int execClusterStartSync(Exec *exec, Connection *conn, std::vector<std::string> args);
        static void connReadClusterStartSyncHandler(Connection *conn);
        // sync doing
        static int doSyncCluster(Connection *conn, std::string data);
        static int execClusterDoSync(Exec *exec, Connection *conn, std::vector<std::string> args);
        static void connReadClusterDoSyncHandler(Connection *conn);
        // sync end
        static int endSyncCluster(Connection *conn);
        static int execClusterEndSync(Exec *exec, Connection *conn, std::vector<std::string> args);
        static void connReadClusterEndSyncHandler(Connection *conn);



        static int pingCluster(Connection *conn);
        static void broadcast(std::string cmd);

        // exec
        // 加入集群

        // 查看集群节点
        static int execClusterNodes(Exec *exec, Connection *conn, std::vector<std::string> args);
        static void *threadProcess(void *arg);

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


#endif //TLBS_CLUSTER_H
