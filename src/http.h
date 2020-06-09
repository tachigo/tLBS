//
// Created by 刘立悟 on 2020/6/5.
//

#ifndef TLBS_HTTP_H
#define TLBS_HTTP_H

#include <vector>
#include <string>
#include <map>


namespace tLBS {

    class Connection;

    typedef int (*execHttpFallback)(Connection *conn, std::vector<std::string> args);

    class Http {
    private:
        static std::map<std::string, Http *> https;

        // eg. "GET /path/to" querystring db=0&table=aaa:bbb&foo=bar 会解析到client的args里边 raw里边的数据会解析到client.args的最后一个里边
        std::string name;
        execHttpFallback fallback;
        std::vector<std::string> params;
        std::string description; // 描述
        bool needSpecifiedDb;
        static void registerHttp(const char *name, execHttpFallback fallback, const char *params, const char *description, bool needSpecifiedDb);
        static int parseQueryBuff(const char *line, std::string *method, std::string *path, std::map<std::string, std::string> *params);
    public:
        Http(const char *name, execHttpFallback fallback, std::vector<std::string> params, const char *description, bool needSpecifiedDb);
        ~Http();
        std::string getName();
        execHttpFallback getFallback();
        std::vector<std::string> getParams();
        bool isNeedSpecifiedDb();


        static bool connIsHttp(std::string query);

        static void init();
        static void free();
        static Http *findHttp(std::string name);
        int call(Connection *conn, std::vector<std::string> args);

        static int processHttp(Connection *conn, std::vector<std::string> args);
        static int processHttpAndReset(Connection *conn, std::string query);
    };
}

#endif //TLBS_HTTP_H
