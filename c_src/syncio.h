//
// Created by liuliwu on 2020-05-11.
//

#ifndef TLBS_SYNCIO_H
#define TLBS_SYNCIO_H

#include <sys/types.h>

ssize_t syncWrite(int fd, char *ptr, ssize_t size, long long timeout);
ssize_t syncRead(int fd, char *ptr, ssize_t size, long long timeout);
ssize_t syncReadLine(int fd, char *ptr, ssize_t size, long long timeout);

#endif //TLBS_SYNCIO_H
