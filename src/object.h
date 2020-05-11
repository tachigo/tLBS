//
// Created by liuliwu on 2020-05-11.
//

#ifndef TLBS_OBJECT_H
#define TLBS_OBJECT_H

#include <climits>

#define OBJ_TYPE_POINTS 0 /* (multi-)point 点 */
#define OBJ_TYPE_LINES 1 /* (multi-)linestring 线 */
#define OBJ_TYPE_POLYGONS 2 /* (multi-)polygon 多边形 */

#define OBJ_FORMAT_DEGREE 0
#define OBJ_FORMAT_RADIAN 1
#define OBJ_FORMAT_CELL_ID 2

#define OBJ_SHARED_REFCOUNT INT_MAX     /* Global object never destroyed. */
#define OBJ_STATIC_REFCOUNT (INT_MAX-1) /* Object allocated in the stack. */
#define OBJ_FIRST_SPECIAL_REFCOUNT OBJ_STATIC_REFCOUNT

typedef struct tLbsObject {
//    unsigned type:4;
//    unsigned encoding:4;
    int refcount;
    void *ptr;
} obj;

const char * getObjectTypeName(obj * o);
void decrRefCount(obj *o);
void decrRefCountVoid(void *o);
void incrRefCount(obj *o);
obj *resetRefCount(obj *obj);

#endif //TLBS_OBJECT_H
