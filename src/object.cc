//
// Created by liuliwu on 2020-05-11.
//

#include "object.h"
#include "zmalloc.h"
#include "common.h"


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