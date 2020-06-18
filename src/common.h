//
// Created by liuliwu on 2020-05-29.
//

#ifndef TLBS_COMMON_H
#define TLBS_COMMON_H

#define C_OK 0
#define C_ERR 1

#define UNUSED(V) ((void) V)

#include <string>
#include <vector>


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


std::vector<std::string> splitString(std::string str, char delimiter);

static int errorCode = 0;
// 定义错误码
#define ERRNO_EXEC_SYNTAX_ERR (++errorCode)
#define ERROR_EXEC_SYNTAX_ERR "命令格式错误"

#define ERRNO_EXEC_CMD_UNKNOWN (++errorCode)
#define ERROR_EXEC_CMD_UNKNOWN "未知的命令"

#define ERRNO_EXEC_HTTP_UNKNOWN (++errorCode)
#define ERROR_EXEC_HTTP_UNKNOWN "未知的HTTP请求"

// 缺参数
#define ERRNO_EXEC_PARAMS_NEED (++errorCode)

#define ERRNO_EXEC_DB_SELECT_ERR (++errorCode)
#define ERROR_EXEC_DB_SELECT_ERR "db不存在"

#define ERRNO_EXEC_TABLE_EXISTS_ERR (++errorCode)
#define ERROR_EXEC_TABLE_EXISTS_ERR "table不存在"

#define ERRNO_EXEC_TABLE_TYPE_ERR (++errorCode)
#define ERROR_EXEC_TABLE_TYPE_ERR "table类型错误"

#define ERRNO_EXEC_TABLE_ENCODING_ERR (++errorCode)
#define ERROR_EXEC_TABLE_ENCODING_ERR "table实现错误"

// s2相关报错
#define ERRNO_EXEC_S2GEOMETRY_ERR (++errorCode)

// mfw相关报错
#define ERRNO_EXEC_MFW_BUSINESS_ERR (++errorCode)

#endif //TLBS_COMMON_H
