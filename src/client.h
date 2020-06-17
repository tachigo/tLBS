//
// Created by 刘立悟 on 2020/5/18.
//

#ifndef TLBS_CLIENT_H
#define TLBS_CLIENT_H

#include <map>
#include <vector>
#include <string>

#include "exec.h"


namespace tLBS {

    class Connection;

    class Client {
    public:
        // exec
        static int execQuit(Exec *exec, Connection *conn, std::vector<std::string> args);
        static int execHello(Exec *exec, Connection *conn, std::vector<std::string> args);
        static int execPing(Exec *exec, Connection *conn, std::vector<std::string> args);
        static int execPong(Exec *exec, Connection *conn, std::vector<std::string> args);
    };
}

#endif //TLBS_CLIENT_H
