//
// Created by 刘立悟 on 2020/6/1.
//

#include "command.h"
#include "connection.h"
#include "client.h"
#include "db.h"
#include "log.h"
#include "t_s2geometry.h"
#include "cluster.h"
#include "server.h"

#include <string>
#include <regex>

using namespace tLBS;

std::map<std::string, Command *> Command::commands;

Command::Command(const char *name, execCmdFallback fallback, int arty, const char *description) {
    this->name = name;
    this->fallback = fallback;
    this->arty = arty;
    this->description = description;
}

Command::~Command() {
//    info("销毁command#") << this->name;
}

void Command::registerCommand(const char *name, tLBS::execCmdFallback fallback, const char *params, const char *description) {
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

execCmdFallback Command::getFallback() {
    return this->fallback;
}

int Command::processCommand(Connection *conn, std::vector<std::string> args) {
//    info(conn->getInfo()) << "执行命令: " << args[0];
    Command *command = Command::findCommand(args[0]);
    if (command == nullptr) {
        // 没有找到命令
        warning("未知的命令: ") << args[0];
        return conn->fail("未知的命令!");
    }
    Server *server = Server::getInstance();
    server->updateCachedTime();
    long long start = server->getUsTime();
    int ret = command->call(conn, args);
    if (ret == C_OK) {
//        long long duration = ustime() - start;
//        char msg[128];
//        sprintf(msg, "命令[%s]内部执行时间: %0.5f 毫秒", args[0].c_str(), (double)duration/(double)1000);
//        info(conn->getInfo()) << msg;
    }
    return ret;
}

std::vector<std::string> Command::parseQueryBuff(const char *line) {
    std::vector<std::string> argv;
    const char *p = line;
    std::string current;
    while (true) {
        while ((*p && isspace(*p)) || *p < 0) {
            // 如果是空格或者不正确的ascii码，指针向前进1
            p++;
        }
        if (*p) {
            // 有非空格字符
            bool inQuotes = false;
            bool inSingleQuotes = false;
            bool done = false;
            while (!done) {
                if (inQuotes) {
                    // 双引号中
                    if (*p == '\\' && *(p+1) == 'x' && isHexDigit(*(p+2)) && isHexDigit(*(p+3))) {
                        unsigned char byte;
                        byte = (hexDigit2int(*(p+2))*16)+
                               hexDigit2int(*(p+3));
                        current += (char *)&byte;
//                        info("8进制数: ") << current;
                        p += 3;
                    }
                    else if (*p == '\\' && *(p+1)) {
                        char c;
                        p++;
                        switch(*p) {
                            case 'n': c = '\n'; break;
                            case 'r': c = '\r'; break;
                            case 't': c = '\t'; break;
                            case 'b': c = '\b'; break;
                            case 'a': c = '\a'; break;
                            default: c = *p; break;
                        }
                        current += &c;
//                        info("转义字符: ") << *p;
                    }
                    else if (*p == '"') {
                        if (*(p+1) && !isspace(*(p+1))) {
                            goto end;
                        }
                        done = true;
//                        info("双引号结束: ") << *p;
                    }
                    else if (!*p) {
                        goto end;
                    }
                    else {
                        current += *p;
//                        info("默认情况: ") << *p;
                    }
                }
                else if (inSingleQuotes) {
                    // 单引号中
                    if (*p == '\\' && *(p+1) == '\'') {
                        p++;
                        current += "'";
//                        info("正常情况: ") << *p;
                    } else if (*p == '\'') {
                        if (*(p+1) && !isspace(*(p+1))) {
                            goto end;
                        }
                        done = true;
//                        info("单引号结束: ") << current;
                    } else if (!*p) {
                        /* unterminated quotes */
                        goto end;
                    } else {
                        current += *p;
//                        info("正常情况: ") << *p;
                    }
                }
                else {
                    switch(*p) {
                        case ' ':
                        case '\n':
                        case '\r':
                        case '\t':
                        case '\0':
                            done = true;
//                            info("结束字符: ") << current;
                            break;
                        case '"':
                            inQuotes = true;
//                            info("进入双引号: ") << current;
                            break;
                        case '\'':
                            inSingleQuotes = true;
//                            info("进入单引号: ") << current;
                            break;
                        default:
                            current += *p;
//                            info("默认情况: ") << *p;
                            break;
                    }

                }
                if (*p) {
                    p++;
                }
            }
            argv.push_back(current);
//            info("argv: ") << current;
            current = "";
        }
        else {
            goto end;
        }
    }
end:
    return argv;
}

int Command::processCommandAndReset(Connection *conn, std::string query) {
    // 解析参数
    std::vector<std::string> args = parseQueryBuff(query.c_str());
    if (args.size() > 0) {
        long long start = ustime();
        if (processCommand(conn, args) == C_OK) {
            long long duration = ustime() - start;
            char msg[1024];
            sprintf(msg, "命令[%s]外部执行时间: %0.5f 毫秒", args[0].c_str(), (double)duration / (double)1000);
            info(conn->getInfo()) << msg;
            return C_OK;
        }
    }
    else {
        error(conn->getInfo()) << "没有参数";
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

int Command::call(Connection *conn, std::vector<std::string> args) {
    int ret = this->fallback(conn, args);
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
    registerCommand("hello", Client::execHello, nullptr, "输出欢迎语");
    registerCommand("quit", Client::execQuit, nullptr, "退出连接");
    registerCommand("db", Db::execDb, nullptr, "查看当前选择的数据库编号");

    // cluster
    registerCommand("clusterjoin", Cluster::execClusterJoin, "address", "有节点要加入集群");
    registerCommand("clusterping", Cluster::execClusterPing, nullptr, "集群ping");

    // s2geometry
    registerCommand("s2test", S2Geometry::execTest, nullptr, "测试s2");
    registerCommand("s2polyset", S2Geometry::execSetPolygon, "table,id,data", "添加一个多边形");
    registerCommand("s2polyget", S2Geometry::execGetPolygon, "table,id", "获取一个多边形");
    registerCommand("s2polydel", S2Geometry::execDelPolygon, "table,id", "删除一个多边形");


}