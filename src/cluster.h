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
    };

    class Cluster {
    private:
        static std::map<std::string, ClusterNode *> nodes;

    public:

        static void init();
        static void free();
        static void addNode(std::string nodeUrl);
        static void addNode(std::string nodeUrl, Connection *conn);

        static void tryConnect();
        static void connConnectHandler(Connection *conn);
        static void connReadHandler(Connection *conn);


        // exec
        // 加入集群
        static int execClusterJoin(Connection *conn, std::vector<std::string> args);
    };
}


#endif //TLBS_CLUSTER_H
