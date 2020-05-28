//
// Created by liuliwu on 2020-05-28.
//

#include "config.h"

#include <gflags/gflags.h>

using namespace tLBS;


Config *Config::instance = nullptr;

Config::Config() {
}

Config* Config::getInstance() {
    if (instance == nullptr) {
        instance = new Config();
    }
    return instance;
}


void Config::init(int *argc, char ***argv) {
    google::ParseCommandLineFlags(argc, argv, true);
    getInstance();
}

