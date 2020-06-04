//
// Created by 刘立悟 on 2020/6/5.
//

#include "http.h"

using namespace tLBS;

std::map<std::string, Http *> Http::https;

bool Http::parseIsHttpRequest(std::vector<std::string> *argv) {
    std::vector<std::string> args;
    args.clear();
    bool isHttp = false;
    if (argv->size() >=3) {
        std::string v1st = (*argv)[0]; // method
        std::string v2nd = (*argv)[1]; // path
        std::string v3rd = (*argv)[2]; // version
        if ((v1st == "GET" || v1st == "POST" || v1st == "PUT" || v1st == "DELETE" || v1st == "HEAD") &&
            v2nd.c_str()[0] == '/' && (v3rd == "HTTP/1.1")) {
            isHttp = true;
            // 最后一个是参数
        }
    }
    if (isHttp) {
        *argv = args;
    }
    return isHttp;
}


void Http::init() {

}