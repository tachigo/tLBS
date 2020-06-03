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
void dumpString(const char *ch);

//char *emptyString();
int isHexDigit(char c);
int hexDigit2int(char c);

long long ustime();
long long mstime();

char *getAbsolutePath(const char *filename);
char *trimString(const char *str1, const char *str2);


// 定义错误码
#define ERRNO_CMD_SYNTAX_ERR 1
#define ERROR_CMD_SYNTAX_ERR "命令格式错误"

#endif //TLBS_COMMON_H
