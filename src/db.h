//
// Created by liuliwu on 2020-05-11.
//

#ifndef TLBS_DB_H
#define TLBS_DB_H

#include "dict.h"
#include "object.h"


#include <map>

namespace tLBS {
    class Db {
    private:
        int id;
        std::map<std::string, void *>dict;

    public:
        Db();
        ~Db();
        int getId();
        Object *lookupKey(Object *keyObj, int flags);
        Object *lookupKeyRead(Object *keyObj);
        Object *lookupKeyWrite(Object *keyObj);
        Object *lookupKeyReadWithFlags(Object *keyObj, int flags);
        Object *lookupKeyWriteWithFlags(Object *keyObj, int flags);
        void add(Object *keyObj, Object *valObj);
        int exists(Object *keyObj);
        int remove(Object *keyObj);
    };
}

typedef struct tLbsDb {
    int id;                     /* Database ID */
    dict *dict;                 /* The keyspace for this DB */
} db;


void dbAdd(db *db, obj *key, obj *val);
int dbExists(db *db, obj *key);
//int dbSyncDelete(db *db, obj *key);
//int dbAsyncDelete(db *db, obj *key);
int dbDelete(db *db, obj *key);
long long dbTotalServerKeyCount();
void dbInit();

#define LOOKUP_NONE 0
#define LOOKUP_NOTOUCH (1<<0)

obj *lookupKey(db *db, obj *key, int flags);
obj *lookupKeyRead(db *db, obj *key);
obj *lookupKeyWrite(db *db, obj *key);

obj *lookupKeyReadWithFlags(db *db, obj *key, int flags);
obj *lookupKeyWriteWithFlags(db *db, obj *key, int flags);

void scanCallback(void *privdata, const dictEntry *de);

#endif //TLBS_DB_H
