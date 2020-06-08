//
// Created by 刘立悟 on 2020/6/5.
//

#include <regex>
#include "http.h"
#include "common.h"
#include "log.h"
#include "client.h"
#include "server.h"
#include "db.h"
#include "t_s2geometry.h"

using namespace tLBS;

std::map<std::string, Http *> Http::https;


int Http::parseQueryBuff(const char *line, std::string *method, std::string *path, std::map<std::string, std::string> *params) {
    std::vector<std::string> argv;
    const char *p = line;
    std::map<std::string, std::string> gets;
    std::string body = "";
    bool done = false;
    while (true) {
        while ((*p && isspace(*p)) || *p < 0) {
            // 如果是空格或者不正确的ascii码，指针向前进1
            p++;
        }
        // 走到末尾
        if (done || *p == '\0') {
            break;
        }
        // Content-Length:
        if (*p == 'C' && *(p+1) == 'o' && *(p+2) == 'n' && *(p+3) == 't' && *(p+4) == 'e' && *(p+5) == 'n' && *(p+6) == 't' &&
            *(p+7) == '-' &&
            *(p+8) == 'L' && *(p+9) == 'e' && *(p+10) == 'n' && *(p+11) == 'g' && *(p+12) == 't' && *(p+13) == 'h') {
            p+=13;
            // \r\n\r\n data起始的位置
            do {
                p++;
            } while (*(p) != '\r' || *(p+1) != '\n' || *(p+2) != '\r' || *(p+3) != '\n');
            p+=4;

            while (*p != '\0') {
                if ((*p && isspace(*p)) || *p < 0) {
                    p++;
                    continue;
                }
                body += *p;
                p++;
            }
            done = true;
        }
        // GET querystring
        if (path->size() > 0 && *p == '?') {
            int i = 1;
            gets.clear();
            bool keyOk = false;
            std::string key = "";
            std::string value = "";
            while (*(p + i)) {
                if (!isspace(*(p + i))) {
                    if (*(p + i) == '&') {
                        // key, value 解析完成
                        if (key.size() > 0) {
                            gets[key] = value;
//                            info(key) << " = " << gets[key];
                        }
                        key = "";
                        value = "";
                        keyOk = false;
                    }
                    else {
                        if (*(p + i) == '=') {
                            if (!keyOk) {
                                keyOk = true;
                            }
                            else {
                                value += *(p + i);
                            }
                        }
                        else {
                            if (!keyOk) {
                                key += *(p + i);
                            }
                            else {
                                value += *(p + i);
                            }
                        }
                    }
                }
                else {
                    // key, value 解析完成
                    if (key.size() > 0) {
                        gets[key] = value;
//                        info(key) << " = " << gets[key];
                    }
                    key = "";
                    value = "";
                    keyOk = false;
                    break;
                }
                i++;
            }
            p += i;
            continue;
        }
        // path
        if (method->size() > 0 && path->size() == 0 &&  *p == '/') {
            int i = 0;
            while (*(p + i)) {
                if (!isspace(*(p + i)) && *(p + i) != '?') {
                    *path += *(p + i);
                    i++;
                }
                else {
                    break;
                }
            }
            p += i;
            continue;
        }
        // method: GET POST PUT DELETE HEAD
        if (method->size() == 0 && *p == 'G' && *(p+1) == 'E' && *(p+2) == 'T' && isspace(*(p+3))) {
            // GET
            *method = "GET";
            p += 3;
            continue;
        }
        if (method->size() == 0 && *p == 'D' && *(p+1) == 'E' && *(p+2) == 'L' && *(p+3) == 'E' && *(p+4) == 'T' && *(p+5) == 'E' && isspace(*(p+6))) {
            // DELETE
            *method = "DELETE";
            p += 6;
            continue;
        }
        if (method->size() == 0 && *p == 'H' && *(p+1) == 'E' && *(p+2) == 'A' && *(p+3) == 'D' && isspace(*(p+4))) {
            // HEAD
            *method = "HEAD";
            p += 4;
            continue;
        }
        if (method->size() == 0 && *p == 'P') {
            if (*(p+1) == 'O' && *(p+2) == 'S' && *(p+3) == 'T' && isspace(*(p+4))) {
                // POST
                *method = "POST";
                p += 4;
                continue;
            }
            if (*(p+1) == 'U' && *(p+2) == 'T' && isspace(*(p+3))) {
                *method = "PUT";
                p += 3;
                continue;
            }
        }
        p++;
    }
    // 打印出调试信息
//    auto mapIter = gets.find("db");
//    if (mapIter == gets.end()) {
//        error("querystring中没有db这个参数: ") << line;
//        return C_ERR;
//    }
//    else {
//        *db = mapIter->second;
//    }
    for (auto mapIter = gets.begin(); mapIter != gets.end(); mapIter++) {
//        info(mapIter->first) << ": " << mapIter->second;
        (*params)[mapIter->first] = mapIter->second;
    }
    (*params)["data"] = body;
    return C_OK;
}


bool Http::clientIsHttp(tLBS::Client *client) {
    if (strlen(client->getQuery()) == 0) {
        return false;
    }
    const char *p = client->getQuery();
    bool isHttp = false;
    std::string method;
    std::string path;
    std::string version;
    bool done = false;
    while (true) {
        // 没有走到末尾
        while ((*p && isspace(*p)) || *p < 0) {
            // 如果是空格或者不正确的ascii码，指针向前进1
            p++;
        }
        if (done || *p == '\0') {
            break;
        }
        if (method.size() > 0 && path.size() > 0 && version.size() > 0) {
//            info("HTTP METHOD: ") << method;
//            info("HTTP PATH: ") << path;
//            info("HTTP VERSION: ") << version;
            isHttp = true;
            done = true;
        }
        // version
        if (method.size() > 0 && path.size() > 0 && version.size() == 0 && *p == 'H') {
            int i = 0;
            while (*(p + i)) {
                if (!isspace(*(p + i))) {
                    version += *(p + i);
                    i++;
                }
                else {
                    break;
                }
            }
            p += i;
            continue;
        }
        // path
        if (method.size() > 0 && path.size() == 0 &&  *p == '/') {
            int i = 0;
            while (*(p + i)) {
                if (!isspace(*(p + i))) {
                    path += *(p + i);
                    i++;
                }
                else {
                    break;
                }
            }
            p += i;
            continue;
        }
        // method: GET POST PUT DELETE HEAD
        if (method.size() == 0 && *p == 'G' && *(p+1) == 'E' && *(p+2) == 'T' && isspace(*(p+3))) {
            // GET
            method = "GET";
            p += 3;
            continue;
        }
        if (method.size() == 0 && *p == 'D' && *(p+1) == 'E' && *(p+2) == 'L' && *(p+3) == 'E' && *(p+4) == 'T' && *(p+5) == 'E' && isspace(*(p+6))) {
            // DELETE
            method = "DELETE";
            p += 6;
            continue;
        }
        if (method.size() == 0 && *p == 'H' && *(p+1) == 'E' && *(p+2) == 'A' && *(p+3) == 'D' && isspace(*(p+4))) {
            // HEAD
            method = "HEAD";
            p += 4;
            continue;
        }
        if (method.size() == 0 && *p == 'P') {
            if (*(p+1) == 'O' && *(p+2) == 'S' && *(p+3) == 'T' && isspace(*(p+4))) {
                // POST
                method = "POST";
                p += 4;
                continue;
            }
            if (*(p+1) == 'U' && *(p+2) == 'T' && isspace(*(p+3))) {
                method = "PUT";
                p += 3;
                continue;
            }
        }
        p++;
    }
    return isHttp;
}


Http::Http(const char *name, tLBS::execHttpFallback fallback, std::vector<std::string> params, const char *description, bool needSpecifiedDb) {
    this->name = name;
    this->fallback = fallback;
    this->params = params;
    this->description = description;
    this->needSpecifiedDb = needSpecifiedDb;
}

Http::~Http() {
    info("销毁http#") << this->name;
}

void Http::registerHttp(const char *name, tLBS::execHttpFallback fallback, const char *params,
                        const char *description, bool needSpecifiedDb) {
    if (params != nullptr) {
        std::string paramsMetadata = params;
        std::regex reg(",");
        std::vector<std::string> v(
                std::sregex_token_iterator(
                        paramsMetadata.begin(), paramsMetadata.end(), reg, -1
                ),
                std::sregex_token_iterator());
        https[name] = new Http(name, fallback, v, description, needSpecifiedDb);
    }
    else {
        std::vector<std::string> v;
        https[name] = new Http(name, fallback, v, description, needSpecifiedDb);
    }
}

std::string Http::getName() {
    return this->name;
}

execHttpFallback Http::getFallback() {
    return this->fallback;
}

std::vector<std::string> Http::getParams() {
    return this->params;
}

bool Http::isNeedSpecifiedDb() {
    return this->needSpecifiedDb;
}

int Http::processHttp(tLBS::Client *client) {
//    info(client->getInfo()) << "执行HTTP请求: " << client->arg(0);
    Http *http = Http::findHttp(client->arg(0));
    if (http == nullptr) {
        // 没有找到命令
        warning("未知的HTTP请求: ") << client->arg(0);
        return client->fail("未知的HTTP请求!");
    }
    Server *server = Server::getInstance();
    server->updateCachedTime();
    long long start = server->getUsTime();
    int ret = http->call(client);
    if (ret == C_OK) {
//        long long duration = ustime() - start;
//        char msg[128];
//        sprintf(msg, "HTTP请求[%s]内部执行时间: %0.5f 毫秒", client->arg(0).c_str(), (double)duration/(double)1000);
//        info(client->getInfo()) << msg;
    }

    return ret;
}


int Http::processHttpAndReset(tLBS::Client *client) {
    // 解析参数
    std::string method = "", path = "";
    std::map<std::string, std::string> params;
    params.clear();
    if (parseQueryBuff(client->getQuery(), &method, &path, &params) == C_OK) {
//        info(client->getInfo()) << " HTTP method: " << method;
//        info(client->getInfo()) << " HTTP path: " << path;
//        for (auto mapIter = params.begin(); mapIter != params.end(); mapIter++) {
//            info(client->getInfo()) << " HTTP GET(" + mapIter->first + "): " << mapIter->second;
//        }
        std::vector<std::string> args;
        args.clear();
        std::string name = method + " " + path;
        args.push_back(name); // name args[0]
        Http *http = findHttp(name);
        if (http == nullptr) {
            // 没有找到命令
            warning("未知的HTTP请求: ") << client->arg(0);
            return client->fail(Json::createErrorJsonObj(ERRNO_EXEC_HTTP_UNKNOWN, ERROR_EXEC_HTTP_UNKNOWN));
        }
        auto mapIter = params.find("db");
        if (mapIter == params.end()) {
            if (http->isNeedSpecifiedDb()) {
                error(client->getInfo()) << " "
                    << http->getName() << " 中必须在querystring中指定db参数";
                return client->fail(Json::createErrorJsonObj(ERRNO_EXEC_PARAMS_NEED, "必须在querystring中指定db参数"));
            }
            else {
                // 默认0 但是其实在创建client的时候就指定了默认0
                client->setDb(Db::getDb(0));
            }
        }
        else {
            // 只要有db参数就设置db
            client->setDb(Db::getDb(atoi(mapIter->second.c_str())));
        }
        // 找到exec 按照params的顺序来设置client的args
        std::vector<std::string> paramsMetadata = http->getParams();
        for (int i = 0; i < paramsMetadata.size(); i++) {
            std::string key = paramsMetadata[i];
            auto mapIter = params.find(key);
            if (mapIter == params.end()) {
                error(client->getInfo()) << " "
                    << http->getName() << " 中必须在querystring中指定" + key + "参数";
                return client->fail(Json::createErrorJsonObj(ERRNO_EXEC_PARAMS_NEED, std::string("必须在querystring中指定" + key + "参数").c_str()));
            }
            else {
                args.push_back(mapIter->second);
            }
        }
        client->setArgs(args);
        if (client->getArgs().size() > 0) {
            long long start = ustime();
            if (processHttp(client) == C_OK) {
                // reset client
//                long long duration = ustime() - start;
//                char msg[1024];
//                sprintf(msg, "HTTP请求[%s]外部执行时间: %0.5f 毫秒", client->arg(0).c_str(), (double)duration / (double)1000);
//                info(client->getInfo()) << msg;
                return C_OK;
            }
        }
        else {
            error(client->getInfo()) << "没有参数";
        }
    }
    else {
        error(client->getInfo()) << "参数解析出错";
        return C_ERR;
    }
    return C_ERR;
}

Http* Http::findHttp(std::string name) {
    auto mapIter = https.find(name);
    if (mapIter != https.end()) {
        return mapIter->second;
    }
    return nullptr;
}

int Http::call(tLBS::Client *client) {
    int ret = this->fallback(client);
    return ret;
}


void Http::free() {
    info("销毁所有http对象");
    // 将所有command命令都销毁
    std::vector<std::string> names;
    for (auto mapIter = https.begin(); mapIter != https.end(); mapIter++) {
        names.push_back(mapIter->first);
    }
    for (int i = 0; i < names.size(); i++) {
        Http *http = https[names[i]];
        https.erase(names[i]);
        delete http;
    }
}


void Http::init() {
    registerHttp("GET /db", Db::execDb, nullptr, "查看当前选择的数据库编号", false);
    registerHttp("GET /s2polyget", S2Geometry::execGetPolygon, "table,id", "获取一个多边形", false);
}