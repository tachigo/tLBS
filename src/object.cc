//
// Created by liuliwu on 2020-05-11.
//

#include "object.h"
#include "zmalloc.h"
#include "common.h"
#include "debug.h"
#include "util.h"
#include "t_string.h"
#include "t_polygon.h"

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
            case OBJ_TYPE_POLYGONS: type = "polygon"; break;
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
        switch(o->type) {
            case OBJ_TYPE_STRING: freeStringObject(o); break;
//            case OBJ_LIST: freeListObject(o); break;
//            case OBJ_SET: freeSetObject(o); break;
//            case OBJ_ZSET: freeZsetObject(o); break;
//            case OBJ_HASH: freeHashObject(o); break;
//            case OBJ_MODULE: freeModuleObject(o); break;
//            case OBJ_STREAM: freeStreamObject(o); break;
            case OBJ_TYPE_POLYGONS: freePolygonObject(o); break;
            default: serverPanic("Unknown object type"); break;
        }
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


int getUnsignedLongLongFromObject(obj *o, unsigned long long *target) {
    unsigned long long value;

    if (o == nullptr) {
        value = 0;
    } else {
//        serverAssertWithInfo(NULL,o,o->type == OBJ_TYPE_STRING);
        if (sdsEncodedObject(o)) {
            if (string2ull((char *)o->ptr,&value) == 0) return C_ERR;
        } else if (o->encoding == OBJ_ENCODING_INT) {
            value = (long)o->ptr;
        } else {
            serverPanic("Unknown string encoding");
        }
    }
    if (target) *target = value;
    return C_OK;
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

int getDoubleFromObject(const obj *o, double *target) {
    double value;

    if (o == nullptr) {
        value = 0;
    } else {
//        serverAssertWithInfo(nullptr,o,o->type == OBJ_TYPE_STRING);
        if (sdsEncodedObject(o)) {
            if (!string2d((char *)o->ptr, sdslen((sds)o->ptr), &value))
                return C_ERR;
        } else if (o->encoding == OBJ_ENCODING_INT) {
            value = (long)o->ptr;
        } else {
            serverPanic("Unknown string encoding");
        }
    }
    *target = value;
    return C_OK;
}

int getLongDoubleFromObject(obj *o, long double *target) {
    long double value;

    if (o == nullptr) {
        value = 0;
    } else {
//        serverAssertWithInfo(NULL,o,o->type == OBJ_TYPE_STRING);
        if (sdsEncodedObject(o)) {
            if (!string2ld((char *)o->ptr, sdslen((sds)o->ptr), &value))
                return C_ERR;
        } else if (o->encoding == OBJ_ENCODING_INT) {
            value = (long)o->ptr;
        } else {
            serverPanic("Unknown string encoding");
        }
    }
    *target = value;
    return C_OK;
}




