//
// Created by liuliwu on 2020-06-10.
//

#ifndef TLBS_CLUSTER_H
#define TLBS_CLUSTER_H

#include <map>
#include <string>

namespace tLBS {

    class Connection;

    class ClusterNode {
    private:
        std::string info;
        std::string ip;
        int port;
        Connection *conn;
        bool established;
    public:
        ClusterNode(std::string ip, int port);
        ~ClusterNode();
        std::string getInfo();
        void setInfo(std::string info);
        std::string getIp();
        int getPort();
        Connection *getConnection();
        void setConnection(Connection *conn);
        void setEstablished(bool established);
        bool getEstablished();
        void closeConnection();
    };

    class Cluster {
    private:
        static std::map<std::string, ClusterNode *> nodes;
        static int unEstablishedNodeCount;
    public:

        static void init();
        static void free();
        static void addNode(std::string nodeUrl);
        static void addNode(std::string nodeUrl, Connection *conn);

        static void incrUnEstablishedNodeCount();
        static void decrUnEstablishedNodeCount();

        static void tryConnect();
        static void connConnectHandler(Connection *conn);
        static void connReadHandler(Connection *conn);
        static void connReadClusterJoinHandler(Connection *conn);
        static void connWriteHandler(Connection *conn);

        static int joinCluster(Connection *conn);

        static void connPingHandler(Connection *conn);

        // exec
        // 加入集群
        static int execClusterJoin(Connection *conn, std::vector<std::string> args);
        // ping
        static int execClusterPing(Connection *conn, std::vector<std::string> args);
        // 查看集群节点
        static int execClusterNodes(Connection *conn, std::vector<std::string> args);
    };
}


#endif //TLBS_CLUSTER_H
