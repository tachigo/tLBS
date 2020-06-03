//
// Created by 刘立悟 on 2020/6/1.
//

#include "command.h"
#include "connection.h"
#include "client.h"
#include "db.h"
#include "log.h"
#include "t_s2geometry.h"

#include <string>

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

void Command::registerCommand(const char *name, tLBS::commandFallback fallback, int arty, const char *description) {
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

int Command::cmdQuit(tLBS::Client *client) {
    Connection *conn = client->getConnection();
    const char *resp = "👋啊朋友再见，啊朋友再见，啊朋友再见吧再见吧~再见吧!👋";
    client->success(resp);
    uint64_t clientFlags = client->getFlags();
    clientFlags |= CLIENT_FLAGS_CLOSE_AFTER_REPLY;
    client->setFlags(clientFlags);
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
    registerCommand("quit", cmdQuit, 0, "退出连接");
    registerCommand("db", Db::cmdDb, 0, "查看当前选择的数据库编号");

    // s2geometry
    registerCommand("s2test", S2Geometry::test, 0, "测试s2");
    registerCommand("s2polyset", S2Geometry::cmdSetPolygon, 3, "添加一个多边形");
    registerCommand("s2polyget", S2Geometry::cmdGetPolygon, 2, "获取一个多边形");
    registerCommand("s2polydel", S2Geometry::cmdDelPolygon, 2, "删除一个多边形");


}