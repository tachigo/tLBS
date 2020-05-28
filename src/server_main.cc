//
// Created by 刘立悟 on 2020/5/18.
//

#include "config.h"
#include "log.h"

int main(int argc, char *argv[]) {
    tLBS::Config::init(&argc, &argv);
    tLBS::Log::init(argv[0]);

    return 0;
}