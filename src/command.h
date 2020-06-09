//
// Created by 刘立悟 on 2020/6/1.
//

#ifndef TLBS_COMMAND_H
#define TLBS_COMMAND_H

#include <vector>
#include <map>
#include <string>

namespace tLBS {
    class Connection;

    typedef int (*execCmdFallback)(Connection *conn, std::vector<std::string> args);

    class Command {
    private:
        static std::map<std::string, Command *> commands;

        std::string name; // 名称
        execCmdFallback fallback;
        int arty; // 参数个数
        std::string description; // 描述
        static std::vector<std::string> parseQueryBuff(const char *line);
        static void registerCommand(const char *name, execCmdFallback fallback, const char *params, const char *description);
    public:
        Command(const char *name, execCmdFallback fallback, int arty, const char *description);
        ~Command();
        std::string getName();
        execCmdFallback getFallback();
        int getArty();
        static void init();
        static void free();
        static Command *findCommand(std::string name);
        int call(Connection *conn, std::vector<std::string> args);


        static int processCommand(Connection *conn, std::vector<std::string> args);
        static int processCommandAndReset(Connection *conn, std::string query);

    };
}


#endif //TLBS_COMMAND_H
