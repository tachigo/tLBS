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
    public:
        // cmd
        static int execQuit(Connection *conn, std::vector<std::string> args);
        static int execHello(Connection *conn, std::vector<std::string> args);
    };
}

#endif //TLBS_CLIENT_H
