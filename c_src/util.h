//
// Created by liuliwu on 2020-05-09.
//

#ifndef TLBS_UTIL_H
#define TLBS_UTIL_H

#include <cstdint>
#include "sds.h"

/* The maximum number of characters needed to represent a long double
 * as a string (long double has a huge range).
 * This should be the size of the buffer given to ld2string */
#define MAX_LONG_DOUBLE_CHARS 5*1024

/* long double to string convertion options */
typedef enum {
    LD_STR_AUTO,     /* %.17Lg */
    LD_STR_HUMAN,    /* %.17Lf + Trimming of trailing zeros */
    LD_STR_HEX       /* %La */
} ld2string_mode;

int stringmatchlen(const char *p, int plen, const char *s, int slen, int nocase);
int stringmatch(const char *p, const char *s, int nocase);
int stringmatchlen_fuzz_test();
long long memtoll(const char *p, int *err);
uint32_t digits10(uint64_t v);
uint32_t sdigits10(int64_t v);
int ll2string(char *s, size_t len, long long value);
int string2ll(const char *s, size_t slen, long long *value);
int string2ull(const char *s, unsigned long long *value);
int string2l(const char *s, size_t slen, long *value);
int string2ld(const char *s, size_t slen, long double *dp);
int string2d(const char *s, size_t slen, double *dp);
int d2string(char *buf, size_t len, double value);
int ld2string(char *buf, size_t len, long double value, ld2string_mode mode);
sds getAbsolutePath(const char *filename);
unsigned long getTimeZone();
int pathIsBaseName(char *path);
void getRandomHexChars(char *p, size_t len);
void getRandomBytes(unsigned char *p, size_t len);

#endif //TLBS_UTIL_H
