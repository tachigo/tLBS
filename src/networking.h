//
// Created by liuliwu on 2020-05-11.
//

#ifndef TLBS_NETWORKING_H
#define TLBS_NETWORKING_H

#include "connection.h"
#include "ae.h"

static void acceptCommonHandler(connection *conn, int flags, char *ip);
void acceptTcpHandler(aeEventLoop *el, int fd, void *privdata, int mask);

#endif //TLBS_NETWORKING_H
