//
// Created by liuliwu on 2020-05-12.
//

#ifndef TLBS_DEBUG_H
#define TLBS_DEBUG_H

#include <unistd.h>

#define serverPanic(...) _serverPanic(__FILE__,__LINE__,__VA_ARGS__),_exit(1)


void _serverPanic(const char *file, int line, const char *msg, ...);
void bugReportStart();

#endif //TLBS_DEBUG_H
