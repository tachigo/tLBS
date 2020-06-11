//
// Created by liuliwu on 2020-05-28.
//

#include "config.h"
#include "log.h"
#include "common.h"

#include <gflags/gflags.h>
#include <fstream>
#include <sstream>

using namespace tLBS;

DEFINE_string(config_file, "", "配置文件的路径");
DEFINE_string(bin_root, "", "可执行文件的根路径");

Config *Config::instance = nullptr;

Config::Config() {
    if (FLAGS_bin_root.size() == 0) {
        FLAGS_bin_root = getAbsolutePath("./");
    }
    if (FLAGS_config_file.size() > 0) {
        const char *configFile = (FLAGS_config_file).c_str();
        if (configFile[0] != '/') {
            // 不是绝对路径，则是相对于bin的路径
            FLAGS_config_file = FLAGS_bin_root + FLAGS_config_file;
        }
        std::ifstream ifs(FLAGS_config_file);
        if (!ifs) {
            if (errno == ENOENT) {
                // 文件不存在
                error("无法打开配置文件" + FLAGS_config_file)
                        << ": " << strerror(errno) << "(" << errno << ")";
            }
            else {
                fatal("无法打开配置文件" + FLAGS_config_file)
                        << ": " << strerror(errno) << "(" << errno << ")";
            }
        }
        else {
            std::string line;
            while (std::getline(ifs, line)) {
//                info(line);
                if (line.size() == 0) {
                    continue;
                }
                const char *p = line.c_str();
                if (p[0] == '#') {
                    continue; // 注释
                }
                std::string key = "";
                std::string val = "";
                while (*p != '=') {
                    key += *p;
                    p++;
                }
                p++;
                while (*p) {
                    val += *p;
                    p++;
                }
                // 解析支持的配置
                if (key == "daemonize") {
                    if (val == "yes") {
                        FLAGS_daemonize = true;
                    }
                    else {
                        FLAGS_daemonize = false;
                    }
                }
                else if (key == "server_hz") {
                    std::istringstream is(val);
                    is >> FLAGS_server_hz;
                }
                else if (key == "tcp_host") {
                    std::istringstream is(val);
                    is >> FLAGS_tcp_host;
                }
                else if (key == "tcp_port") {
                    std::istringstream is(val);
                    is >> FLAGS_tcp_port;
                }
                else if (key == "tcp_backlog") {
                    std::istringstream is(val);
                    is >> FLAGS_tcp_backlog;
                }
                else if (key == "tcp_keepalive") {
                    std::istringstream is(val);
                    is >> FLAGS_tcp_keepalive;
                }
                else if (key == "max_connections") {
                    std::istringstream is(val);
                    is >> FLAGS_max_connections;
                }
                else if (key == "threads_connection") {
                    std::istringstream is(val);
                    is >> FLAGS_threads_connection;
                }
                else if (key == "db_num") {
                    std::istringstream is(val);
                    is >> FLAGS_db_num;
                }
                else if (key == "cluster_nodes") {
                    FLAGS_cluster_nodes = val;
                }
            }
        }
    }
}

void Config::free() {
    delete instance;
}

Config::~Config() {

}

Config *Config::getInstance() {
    if (instance == nullptr) {
        instance = new Config();
    }
    return instance;
}


void Config::init(int *argc, char ***argv) {
    google::ParseCommandLineFlags(argc, argv, true);
    getInstance();
}

