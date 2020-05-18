//
// Created by liuliwu on 2020-05-15.
//

#include "t_string.h"
#include "sds.h"
#include "zmalloc.h"
#include "debug.h"
#include "server.h"
#include "util.h"
#include <cstring>

/* Create a string object with encoding OBJ_ENCODING_RAW, that is a plain
 * string object where o->ptr points to a proper sds string. */
obj *createRawStringObject(const char *ptr, size_t len) {
    return createObject(OBJ_TYPE_STRING, sdsnewlen(ptr,len));
}

/* Create a string object with encoding OBJ_ENCODING_EMBSTR, that is
 * an object where the sds string is actually an unmodifiable string
 * allocated in the same chunk as the object itself. */
obj *createEmbeddedStringObject(const char *ptr, size_t len) {
    obj *o = (obj *)zmalloc(sizeof(obj)+sizeof(struct sdshdr8)+len+1);
    struct sdshdr8 *sh = (sdshdr8 *)(void*)(o+1);

    o->type = OBJ_TYPE_STRING;
    o->encoding = OBJ_ENCODING_EMBSTR;
    o->ptr = sh+1;
    o->refcount = 1;
//    if (server.maxmemory_policy & MAXMEMORY_FLAG_LFU) {
//        o->lru = (LFUGetTimeInMinutes()<<8) | LFU_INIT_VAL;
//    } else {
//        o->lru = LRU_CLOCK();
//    }

    sh->len = len;
    sh->alloc = len;
    sh->flags = SDS_TYPE_8;
    if (ptr == SDS_NOINIT)
        sh->buf[len] = '\0';
    else if (ptr) {
        memcpy(sh->buf,ptr,len);
        sh->buf[len] = '\0';
    } else {
        memset(sh->buf,0,len+1);
    }
    return o;
}

/* Create a string object with EMBSTR encoding if it is smaller than
 * OBJ_ENCODING_EMBSTR_SIZE_LIMIT, otherwise the RAW encoding is
 * used.
 *
 * The current limit of 44 is chosen so that the biggest string object
 * we allocate as EMBSTR will still fit into the 64 byte arena of jemalloc. */
#define OBJ_ENCODING_EMBSTR_SIZE_LIMIT 44
obj *createStringObject(const char *ptr, size_t len) {
    if (len <= OBJ_ENCODING_EMBSTR_SIZE_LIMIT)
        return createEmbeddedStringObject(ptr,len);
    else
        return createRawStringObject(ptr,len);
}


/* Create a string object from a long long value. When possible returns a
 * shared integer object, or at least an integer encoded one.
 *
 * If valueobj is non zero, the function avoids returning a a shared
 * integer, because the object is going to be used as value in the Redis key
 * space (for instance when the INCR command is used), so we want LFU/LRU
 * values specific for each key. */
obj *createStringObjectFromLongLongWithOptions(long long value, int valueobj) {
    obj *o;

//    if (server.maxmemory == 0 ||
//        !(server.maxmemory_policy & MAXMEMORY_FLAG_NO_SHARED_INTEGERS))
//    {
//        /* If the maxmemory policy permits, we can still return shared integers
//         * even if valueobj is true. */
//        valueobj = 0;
//    }

    if (value >= 0 && value < OBJ_SHARED_INTEGERS && valueobj == 0) {
        incrRefCount(shared.integers[value]);
        o = shared.integers[value];
    } else {
        if (value >= LONG_MIN && value <= LONG_MAX) {
            o = createObject(OBJ_TYPE_STRING, nullptr);
            o->encoding = OBJ_ENCODING_INT;
            o->ptr = (void*)((long)value);
        } else {
            o = createObject(OBJ_TYPE_STRING,sdsfromlonglong(value));
        }
    }
    return o;
}

/* Wrapper for createStringObjectFromLongLongWithOptions() always demanding
 * to create a shared object if possible. */
obj *createStringObjectFromLongLong(long long value) {
    return createStringObjectFromLongLongWithOptions(value,0);
}

/* Wrapper for createStringObjectFromLongLongWithOptions() avoiding a shared
 * object when LFU/LRU info are needed, that is, when the object is used
 * as a value in the key space, and Redis is configured to evict based on
 * LFU/LRU. */
obj *createStringObjectFromLongLongForValue(long long value) {
    return createStringObjectFromLongLongWithOptions(value,1);
}

/* Create a string object from a long double. If humanfriendly is non-zero
 * it does not use exponential format and trims trailing zeroes at the end,
 * however this results in loss of precision. Otherwise exp format is used
 * and the output of snprintf() is not modified.
 *
 * The 'humanfriendly' option is used for INCRBYFLOAT and HINCRBYFLOAT. */
obj *createStringObjectFromLongDouble(long double value, int humanfriendly) {
    char buf[MAX_LONG_DOUBLE_CHARS];
    int len = ld2string(buf,sizeof(buf),value,humanfriendly? LD_STR_HUMAN: LD_STR_AUTO);
    return createStringObject(buf,len);
}

/* Duplicate a string object, with the guarantee that the returned object
 * has the same encoding as the original one.
 *
 * This function also guarantees that duplicating a small integer object
 * (or a string object that contains a representation of a small integer)
 * will always result in a fresh object that is unshared (refcount == 1).
 *
 * The resulting object always has refcount set to 1. */
obj *dupStringObject(const obj *o) {
    obj *d;

//    serverAssert(o->type == OBJ_TYPE_STRING);

    switch(o->encoding) {
        case OBJ_ENCODING_RAW:
            return createRawStringObject((const char *)o->ptr,sdslen((sds)o->ptr));
        case OBJ_ENCODING_EMBSTR:
            return createEmbeddedStringObject((const char *)o->ptr,sdslen((sds)o->ptr));
        case OBJ_ENCODING_INT:
            d = createObject(OBJ_TYPE_STRING, nullptr);
            d->encoding = OBJ_ENCODING_INT;
            d->ptr = o->ptr;
            return d;
        default:
            serverPanic("Wrong encoding.");
            break;
    }
}


void freeStringObject(obj *o) {
    if (o->encoding == OBJ_ENCODING_RAW) {
        sdsfree((sds)o->ptr);
    }
}