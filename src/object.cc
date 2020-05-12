//
// Created by liuliwu on 2020-05-11.
//

#include "object.h"
#include "zmalloc.h"
#include "common.h"
#include "sds.h"
#include "debug.h"
#include "util.h"
#include <cstring>

const char * getObjectTypeName(obj * o) {
    const char* type;
    if (o == nullptr) {
        type = "none";
    } else {
        switch(o->type) {
            case OBJ_TYPE_STRING: type = "string"; break;
//            case OBJ_LIST: type = "list"; break;
//            case OBJ_SET: type = "set"; break;
//            case OBJ_ZSET: type = "zset"; break;
//            case OBJ_HASH: type = "hash"; break;
//            case OBJ_STREAM: type = "stream"; break;
//            case OBJ_MODULE: {
//                moduleValue *mv = o->ptr;
//                type = mv->type->name;
//            }; break;
            default: type = "unknown"; break;
        }
    }
    return type;
}

obj *createObject(int type, void *ptr) {
    obj *o = (obj *)zmalloc(sizeof(*o));
    o->type = type;
    o->encoding = OBJ_ENCODING_RAW;
    o->ptr = ptr;
    o->refcount = 1;

    /* Set the LRU to the current lruclock (minutes resolution), or
     * alternatively the LFU counter. */
//    if (server.maxmemory_policy & MAXMEMORY_FLAG_LFU) {
//        o->lru = (LFUGetTimeInMinutes()<<8) | LFU_INIT_VAL;
//    } else {
//        o->lru = LRU_CLOCK();
//    }
    return o;
}

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


obj *makeObjectShared(obj *o) {
//    serverAssert(o->refcount == 1);
    o->refcount = OBJ_SHARED_REFCOUNT;
    return o;
}


void incrRefCount(obj *o) {
    if (o->refcount < OBJ_FIRST_SPECIAL_REFCOUNT) {
        o->refcount++;
    } else {
        if (o->refcount == OBJ_SHARED_REFCOUNT) {
            /* Nothing to do: this refcount is immutable. */
        } else if (o->refcount == OBJ_STATIC_REFCOUNT) {
//            serverPanic("You tried to retain an object allocated in the stack");
            serverLog(LL_WARNING, "You tried to retain an object allocated in the stack");
        }
    }
}

void decrRefCount(obj *o) {
    if (o->refcount == 1) {
//        switch(o->type) {
//            case OBJ_STRING: freeStringObject(o); break;
//            case OBJ_LIST: freeListObject(o); break;
//            case OBJ_SET: freeSetObject(o); break;
//            case OBJ_ZSET: freeZsetObject(o); break;
//            case OBJ_HASH: freeHashObject(o); break;
//            case OBJ_MODULE: freeModuleObject(o); break;
//            case OBJ_STREAM: freeStreamObject(o); break;
//            default: serverPanic("Unknown object type"); break;
//        }
        zfree(o);
    } else {
//        if (o->refcount <= 0) serverPanic("decrRefCount against refcount <= 0");
        if (o->refcount <= 0) serverLog(LL_WARNING, "decrRefCount against refcount <= 0");
        if (o->refcount != OBJ_SHARED_REFCOUNT) o->refcount--;
    }
}

/* This variant of decrRefCount() gets its argument as void, and is useful
 * as free method in data structures that expect a 'void free_object(void*)'
 * prototype for the free method. */
void decrRefCountVoid(void *o) {
    decrRefCount((obj *)o);
}

/* This function set the ref count to zero without freeing the object.
 * It is useful in order to pass a new object to functions incrementing
 * the ref count of the received object. Example:
 *
 *    functionThatWillIncrementRefCount(resetRefCount(CreateObject(...)));
 *
 * Otherwise you need to resort to the less elegant pattern:
 *
 *    *obj = createObject(...);
 *    functionThatWillIncrementRefCount(obj);
 *    decrRefCount(obj);
 */
obj *resetRefCount(obj *obj) {
    obj->refcount = 0;
    return obj;
}


int getLongLongFromObject(obj *o, long long *target) {
    long long value;

    if (o == nullptr) {
        value = 0;
    } else {
//        serverAssertWithInfo(NULL,o,o->type == OBJ_TYPE_STRING);
        if (sdsEncodedObject(o)) {
            if (string2ll((char *)o->ptr,sdslen((sds)o->ptr),&value) == 0) return C_ERR;
        } else if (o->encoding == OBJ_ENCODING_INT) {
            value = (long)o->ptr;
        } else {
            serverPanic("Unknown string encoding");
        }
    }
    if (target) *target = value;
    return C_OK;
}




