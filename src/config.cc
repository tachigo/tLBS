//
// Created by liuliwu on 2020-05-28.
//


#include "config.h"

tLBS::Config *tLBS::Config::instance = nullptr;

tLBS::Config::Config() {
}

tLBS::Config* tLBS::Config::getInstance() {
    if (instance == nullptr) {
        instance = new Config();
    }
    return instance;
}


void tLBS::Config::init(int *argc, char ***argv) {
    google::ParseCommandLineFlags(argc, argv, true);
    getInstance();
}