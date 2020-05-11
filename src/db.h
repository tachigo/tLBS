//
// Created by liuliwu on 2020-05-11.
//

#ifndef TLBS_DB_H
#define TLBS_DB_H

#include "dict.h"
#include "object.h"

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


#endif //TLBS_DB_H
