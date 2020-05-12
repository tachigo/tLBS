//
// Created by liuliwu on 2020-05-11.
//

#ifndef TLBS_COMMON_H
#define TLBS_COMMON_H

#include <cstdint>

typedef long long mstime_t; /* millisecond time type. */
typedef long long ustime_t; /* microsecond time type. */

/* Error codes */
#define C_OK                    0
#define C_ERR                   -1

/* Anti-warning macro... */
#define UNUSED(V) ((void) V)

/* Utils */
long long ustime();
long long mstime();


extern double R_Zero, R_PosInf, R_NegInf, R_Nan;

#ifdef __GNUC__
void serverLog(int level, const char *fmt, ...)
__attribute__((format(printf, 2, 3)));
#else
void serverLog(int level, const char *fmt, ...);
#endif
void serverLogRaw(int level, const char *msg);
void serverLogFromHandler(int level, const char *msg);

/* Log levels */
#define LL_DEBUG 0
#define LL_VERBOSE 1
#define LL_NOTICE 2
#define LL_WARNING 3
#define LL_RAW (1<<10) /* Modifier to log without timestamp */
#define LOG_MAX_LEN    1024 /* Default maximum length of syslog messages.*/


uint64_t dictSdsCaseHash(const void *key);

int dictSdsKeyCaseCompare(void *privdata, const void *key1,
                          const void *key2);

void dictSdsDestructor(void *privdata, void *val);

uint64_t dictSdsHash(const void *key);
int dictSdsKeyCompare(void *privdata, const void *key1,
                      const void *key2);

void dictObjectDestructor(void *privdata, void *val);

#endif //TLBS_COMMON_H
