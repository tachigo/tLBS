//
// Created by liuliwu on 2020-05-15.
//

#ifndef TLBS_T_STRING_H
#define TLBS_T_STRING_H

#include "object.h"
#include <sys/types.h>


obj *createStringObject(const char *ptr, size_t len);
obj *createRawStringObject(const char *ptr, size_t len);
obj *createEmbeddedStringObject(const char *ptr, size_t len);
obj *createStringObjectFromLongLongWithOptions(long long value, int valueobj);
obj *createStringObjectFromLongLong(long long value);
obj *createStringObjectFromLongLongForValue(long long value);
obj *createStringObjectFromLongDouble(long double value, int humanfriendly);
obj *dupStringObject(const obj *o);
void freeStringObject(obj *o);

#endif //TLBS_T_STRING_H
