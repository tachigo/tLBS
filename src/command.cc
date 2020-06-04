//
// Created by 刘立悟 on 2020/6/1.
//

#include "command.h"
#include "connection.h"
#include "client.h"
#include "db.h"
#include "log.h"
#include "t_s2geometry.h"
#include "server.h"

#include <string>
#include <regex>

using namespace tLBS;

std::map<std::string, Command *> Command::commands;

Command::Command(const char *name, commandFallback fallback, int arty, const char *description) {
    this->name = name;
    this->fallback = fallback;
    this->arty = arty;
    this->description = description;
}

Command::~Command() {
    info("销毁command#") << this->name;
}

void Command::registerCommand(const char *name, tLBS::commandFallback fallback, const char *params, const char *description) {
    int arty = 0;
    if (params != nullptr) {
        std::string paramsMetadata = params;
        std::regex reg(",");
        std::vector<std::string> v(
                std::sregex_token_iterator(
                        paramsMetadata.begin(), paramsMetadata.end(), reg, -1
                ),
                std::sregex_token_iterator());
        arty = v.size();
    }
    commands[name] = new Command(name, fallback, arty, description);
}

std::string Command::getName() {
    return this->name;
}

int Command::getArty() {
    return this->arty;
}

commandFallback Command::getFallback() {
    return this->fallback;
}

int Command::processCommand(Client *client) {
    info(client->getInfo()) << "执行命令: " << client->arg(0);
    Command *command = Command::findCommand(client->arg(0));
    if (command == nullptr) {
        // 没有找到命令
        warning("未知的命令: ") << client->arg(0);
        return client->fail("未知的命令!");
    }
    Server *server = Server::getInstance();
    server->updateCachedTime();
    long long start = server->getUsTime();
    int ret = command->call(client);
    if (ret == C_OK) {
        long long duration = ustime() - start;
        char msg[128];
        sprintf(msg, "命令[%s]内部执行时间: %0.5f 毫秒", client->arg(0).c_str(), (double)duration/(double)1000);
        info(client->getInfo()) << msg;
    }
    return ret;
}

int Command::processCommandAndReset(Client *client) {
    if (processCommand(client) == C_OK) {
        // reset client
        return C_OK;
    }
    return C_ERR;
}


Command* Command::findCommand(std::string name) {
    auto mapIter = commands.find(name);
    if (mapIter != commands.end()) {
        return mapIter->second;
    }
    return nullptr;
}

int Command::call(tLBS::Client *client) {
    int ret = this->fallback(client);
    return ret;
}


void Command::free() {
    info("销毁所有command对象");
    // 将所有command命令都销毁
    std::vector<std::string> names;
    for (auto mapIter = commands.begin(); mapIter != commands.end(); mapIter++) {
        names.push_back(mapIter->first);
    }
    for (int i = 0; i < names.size(); i++) {
        Command *command = commands[names[i]];
        commands.erase(names[i]);
        delete command;
    }
}

void Command::init() {
    registerCommand("quit", Client::cmdQuit, nullptr, "退出连接");
    registerCommand("db", Db::cmdDb, nullptr, "查看当前选择的数据库编号");

    // s2geometry
    registerCommand("s2test", S2Geometry::test, nullptr, "测试s2");
    registerCommand("s2polyset", S2Geometry::cmdSetPolygon, "table,id,data", "添加一个多边形");
    registerCommand("s2polyget", S2Geometry::cmdGetPolygon, "table,id", "获取一个多边形");
    registerCommand("s2polydel", S2Geometry::cmdDelPolygon, "table,id", "删除一个多边形");


}