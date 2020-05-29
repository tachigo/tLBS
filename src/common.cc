//
// Created by liuliwu on 2020-05-29.
//

#include "common.h"
#include <sys/time.h>

void getTimeval(long *seconds, long *milliseconds) {
    struct timeval tv = {0, 0};
    gettimeofday(&tv, nullptr);
    *seconds = tv.tv_sec;
    *milliseconds = tv.tv_usec / 1000;
}

void addMillisecondsToNow(long long milliseconds, long *sec, long *ms) {
    long curSec, curMs, whenSec, whenMs;
    getTimeval(&curSec, &curMs);
    whenSec = curSec + milliseconds / 1000;
    whenMs = curMs + milliseconds % 1000;
    if (whenMs >= 1000) {
        whenSec += 1;
        whenMs -= 1000;
    }
    *sec = whenSec;
    *ms = whenMs;
}