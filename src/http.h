//
// Created by 刘立悟 on 2020/6/5.
//

#ifndef TLBS_HTTP_H
#define TLBS_HTTP_H

#include <vector>
#include <string>
#include <map>


namespace tLBS {

    class Client;

    typedef int (*httpFallback)(Client *client);

    class Http {
    private:
        static std::map<std::string, Http *> https;

        // eg. "GET:/path/to" querystring db=0&table=aaa:bbb&foo=bar 会解析到client的args里边 raw里边的数据会解析到client.args的最后一个里边
        std::string name;
        httpFallback fallback;
        std::vector<std::string> params;
        std::string description; // 描述
        static void registerHttp(const char *name, httpFallback fallback, const char *params, const char *description);

    public:
        Http(const char *name, httpFallback fallback, const char *params, const char *description);
        ~Http();
        std::string getName();
        httpFallback getFallback();

        static bool parseIsHttpRequest(std::vector<std::string> *argv);

        static void init();
        static void free();
        static Http *findHttp(std::string name);
        int call(Client *client);

        static int processHttp(Client *client);
        static int processHttpAndReset(Client *);
    };
}

#endif //TLBS_HTTP_H
