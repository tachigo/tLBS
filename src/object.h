//
// Created by liuliwu on 2020-05-11.
//

#ifndef TLBS_OBJECT_H
#define TLBS_OBJECT_H

#include <climits>
#include <cctype>

#define OBJ_TYPE_STRING 0    /* String object. */
#define OBJ_TYPE_POINTS 0 /* (multi-)point 点 */
#define OBJ_TYPE_LINES 1 /* (multi-)linestring 线 */
#define OBJ_TYPE_POLYGONS 2 /* (multi-)polygon 多边形 */

/* Objects encoding. Some kind of objects like Strings and Hashes can be
 * internally represented in multiple ways. The 'encoding' field of the object
 * is set to one of this fields for this object. */
#define OBJ_ENCODING_RAW 0     /* Raw representation */
#define OBJ_ENCODING_INT 1     /* Encoded as integer */
#define OBJ_ENCODING_HT 2      /* Encoded as hash table */
#define OBJ_ENCODING_ZIPMAP 3  /* Encoded as zipmap */
#define OBJ_ENCODING_LINKEDLIST 4 /* No longer used: old list encoding. */
#define OBJ_ENCODING_ZIPLIST 5 /* Encoded as ziplist */
#define OBJ_ENCODING_INTSET 6  /* Encoded as intset */
#define OBJ_ENCODING_SKIPLIST 7  /* Encoded as skiplist */
#define OBJ_ENCODING_EMBSTR 8  /* Embedded sds string encoding */
#define OBJ_ENCODING_QUICKLIST 9 /* Encoded as linked list of ziplists */
#define OBJ_ENCODING_STREAM 10 /* Encoded as a radix tree of listpacks */

#define OBJ_ENCODING_S2INDEX 11 /* Encoded as a s2index */

#define OBJ_FORMAT_DEGREE 0
#define OBJ_FORMAT_RADIAN 1
#define OBJ_FORMAT_CELL_ID 2

#define OBJ_SHARED_REFCOUNT INT_MAX     /* Global object never destroyed. */
#define OBJ_STATIC_REFCOUNT (INT_MAX-1) /* Object allocated in the stack. */
#define OBJ_FIRST_SPECIAL_REFCOUNT OBJ_STATIC_REFCOUNT

typedef struct tLbsObject {
    unsigned type:4;
    unsigned encoding:4;
    int refcount;
    void *ptr;
} obj;

const char * getObjectTypeName(obj * o);

obj *createObject(int type, void *ptr);
obj *createStringObject(const char *ptr, size_t len);
obj *createRawStringObject(const char *ptr, size_t len);
obj *createEmbeddedStringObject(const char *ptr, size_t len);
obj *makeObjectShared(obj *o);

void decrRefCount(obj *o);
void decrRefCountVoid(void *o);
void incrRefCount(obj *o);
obj *resetRefCount(obj *obj);

#define sdsEncodedObject(objptr) (objptr->encoding == OBJ_ENCODING_RAW || objptr->encoding == OBJ_ENCODING_EMBSTR)


int getLongLongFromObject(obj *o, long long *target);
int getUnsignedLongLongFromObject(obj *o, unsigned long long *target);
int getDoubleFromObject(const obj *o, double *target);
int getLongDoubleFromObject(obj *o, long double *target);



#endif //TLBS_OBJECT_H
