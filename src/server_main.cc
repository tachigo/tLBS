//
// Created by 刘立悟 on 2020/5/18.
//

#include "config.h"
#include "log.h"
#include "server.h"
#include "el.h"

using namespace tLBS;

int main(int argc, char *argv[]) {
    Config::init(&argc, &argv); // 根据命令行参数初始化配置
    Log::init(argv[0]); // 初始化日志
    warning("oO0OoO0OoO0Oo tLBS-server is starting oO0OoO0OoO0Oo");
    auto *server = new Server();
    server->init(); // 初始化服务器
    // i/o多路复用代理
    auto *el = new EventLoop(10);
    el->start();
    return 0;
}