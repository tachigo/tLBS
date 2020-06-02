//
// Created by liuliwu on 2020-05-29.
//

#include "common.h"
#include "log.h"
#include <sys/time.h>
#include <string>

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

void dumpString(const char *ch) {
    for (int i = 0; i < strlen(ch); i++) {
        char msg[32];
        snprintf(msg, sizeof(msg), "%d -- %d : %c", i, *(ch + i), *(ch + i));
        info(msg);
    }
}

//char *emptyString() {
//    char ch[0];
//    ch[1] = '\0';
//    return ch;
//}

int isHexDigit(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
           (c >= 'A' && c <= 'F');
}

int hexDigit2int(char c) {
    switch(c) {
        case '0': return 0;
        case '1': return 1;
        case '2': return 2;
        case '3': return 3;
        case '4': return 4;
        case '5': return 5;
        case '6': return 6;
        case '7': return 7;
        case '8': return 8;
        case '9': return 9;
        case 'a': case 'A': return 10;
        case 'b': case 'B': return 11;
        case 'c': case 'C': return 12;
        case 'd': case 'D': return 13;
        case 'e': case 'E': return 14;
        case 'f': case 'F': return 15;
        default: return 0;
    }
}

/* Return the UNIX time in microseconds */
long long ustime() {
    struct timeval tv;
    long long ust;

    gettimeofday(&tv, nullptr);
    ust = ((long long)tv.tv_sec)*1000000;
    ust += tv.tv_usec;
    return ust;
}

/* Return the UNIX time in milliseconds */
long long mstime() {
    return ustime()/1000;
}