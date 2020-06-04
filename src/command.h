//
// Created by 刘立悟 on 2020/6/1.
//

#ifndef TLBS_COMMAND_H
#define TLBS_COMMAND_H

#include <vector>
#include <map>
#include <string>

namespace tLBS {
    class Client;

    typedef int (*commandFallback)(Client *client);

    class Command {
    private:
        static std::map<std::string, Command *> commands;

        std::string name; // 名称
        commandFallback fallback;
        int arty; // 参数个数
        std::string description; // 描述
        static void registerCommand(const char *name, commandFallback fallback, const char *params, const char *description);
    public:
        Command(const char *name, commandFallback fallback, int arty, const char *description);
        ~Command();
        std::string getName();
        commandFallback getFallback();
        int getArty();
        static void init();
        static void free();
        static Command *findCommand(std::string name);
        int call(Client *client);


        static int processCommand(Client *client);
        static int processCommandAndReset(Client *);

    };
}


#endif //TLBS_COMMAND_H
