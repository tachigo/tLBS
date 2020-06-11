//
// Created by liuliwu on 2020-05-29.
//

#include "common.h"
#include "log.h"
#include <sys/time.h>
#include <string>
#include <regex>

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

char *trimString(const char *str1, const char *str2) {
    std::string str = str1;
    size_t n = str.find_last_not_of(str2);
    str.erase(n + 1, str.size() - n);

    n = str.find_first_not_of(str2);
    str.erase(0, n);
    return (char *)str.c_str();
}


char *getAbsolutePath(const char *filename) {
    char cwd[1024];
    std::string absPath;
    std::string relPath = trimString(filename, " \r\n\t");
    if (relPath[0] == '/') {
        return (char *)relPath.c_str();
    }
    if (getcwd(cwd, sizeof(cwd)) == nullptr) {
        return nullptr;
    }
    absPath = cwd;
    if (absPath.size() > 0 && absPath[absPath.size() - 1] != '/') {
        // 如果最后一个字符是不是/ 在后边添加/
        absPath += "/";
    }
    if (relPath.size() >= 2 &&
        relPath[0] == '.' && relPath[1] == '/') {
        // ./开头的
        relPath = relPath.substr(2, relPath.size() - 2);
    }

//    info("准备好的absPath: ") << absPath;

    while (true) {
        if (relPath.size() >= 3 &&
            relPath[0] == '.' && relPath[1] == '.' && relPath[2] == '/') {
            // ../开头这种形式
//            info("relPath: ") << relPath;
            relPath = relPath.substr(3, relPath.size() - 3);
//            info("relPath: ") << relPath;
            if (absPath.size() > 1) {
                // 从absPath最后一个位置开始往前截断
                int p = absPath.size() - 2;
                int trimLen = 1;
                while (absPath[p] != '/') {
                    p--;
                    trimLen++;
                }
                absPath = absPath.substr(0, absPath.size() - trimLen);
//                info("absPath:") << absPath;
            }
        }
        else if (relPath.size() > 0) {
            // 如果是指定的路径名，则往absPath上添加
            int p = 0;
            while (relPath[p] != '/' && p < relPath.size()) {
                p++;
            }
            if (relPath[p] == '\0') {
                // 判断是否到字符串最后一个字符了 说明这个relPath字符串里已经没有/了
                break;
            }
            absPath = absPath + relPath.substr(0, p + 1);
//            info("absPath: ") << absPath;
            relPath = relPath.substr(p + 1, relPath.size() - p);
//            info("relPath: ") << relPath;
        }
        else {
            break;
        }
    }
    // 上面的while循环执行完成以后 relPath 要么size() == 0 要么 是不包含以/开头的部分
//    info("absPath: ") << absPath;
//    info("relPath: ") << relPath;
    absPath = absPath + relPath;
//    info("absPath: ") << absPath;
    return (char *)absPath.c_str();
}


std::vector<std::string> splitString(std::string str, std::string delimiter) {
    std::regex reg(delimiter);
    std::vector<std::string> v(
            std::sregex_token_iterator(
                    str.begin(), str.end(), reg, -1
            ),
            std::sregex_token_iterator());
    return v;
}