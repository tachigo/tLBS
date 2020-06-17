//
// Created by 刘立悟 on 2020/6/5.
//

#ifndef TLBS_HTTP_H
#define TLBS_HTTP_H

#include <vector>
#include <string>
#include <map>

#include "exec.h"


namespace tLBS {

    class Connection;

    typedef int (*execHttpFallback)(Exec *exec, Connection *conn, std::vector<std::string> args);

    class Http: public Exec {
    private:
        static std::map<std::string, Http *> https;

        // eg. "GET /path/to" querystring db=0&table=aaa:bbb&foo=bar 会解析到client的args里边 raw里边的数据会解析到client.args的最后一个里边
        std::string name;
        execHttpFallback fallback;
        std::vector<std::string> params;
        std::string description; // 描述
        bool needSpecifiedDb;
        bool clusterBroadcast; // 是否集群广播


        static void registerHttp(const char *name, execHttpFallback fallback, const char *params, const char *description, bool needSpecifiedDb, bool needClusterBroadcast);
        static int parseQueryBuff(const char *line, std::string *method, std::string *path, std::map<std::string, std::string> *params);
    public:
        Http(const char *name, execHttpFallback fallback, std::vector<std::string> params, const char *description, bool needSpecifiedDb, bool needClusterBroadcast);
        ~Http();
        std::string getName();
        execHttpFallback getFallback();
        std::vector<std::string> getParams();
        bool isNeedSpecifiedDb();
        void setNeedClusterBroadcast(bool needClusterBroadcast);
        bool getNeedClusterBroadcast();


        static bool connIsHttp(std::string query);

        static void init();
        static void free();
        static Http *findHttp(std::string name);
        int call(Connection *conn, std::vector<std::string> args);

        static int processHttp(Connection *conn, std::vector<std::string> args, bool inClusterScope);
        static int processHttpAndReset(Connection *conn, std::string query, bool inClusterScope);
    };
}

#endif //TLBS_HTTP_H
