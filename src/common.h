//
// Created by liuliwu on 2020-05-29.
//

#ifndef TLBS_COMMON_H
#define TLBS_COMMON_H

#define C_OK 0
#define C_ERR 1

#define UNUSED(V) ((void) V)


void getTimeval(long *seconds, long *milliseconds);
void addMillisecondsToNow(long long milliseconds, long *sec, long *ms);


#endif //TLBS_COMMON_H
