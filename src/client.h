//
// Created by 刘立悟 on 2020/5/18.
//

#ifndef TLBS_CLIENT_H
#define TLBS_CLIENT_H

#include <map>
#include <vector>
#include <string>

namespace tLBS {

    class Connection;

    class Client {

    private:
        static std::map<uint64_t, Client *> clients;

        Connection *conn;
    public:
        explicit Client(Connection *conn);
        ~Client();
        Connection *getConnection();
        static void linkClient(Client *client);
        static void unlinkClient(Client *client);
        static void unlinkIfNeed();

        // cmd
        static int execQuit(Connection *conn, std::vector<std::string> args);
        static int execHello(Connection *conn, std::vector<std::string> args);
    };
}

#endif //TLBS_CLIENT_H
