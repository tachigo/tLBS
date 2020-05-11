//
// Created by liuliwu on 2020-05-11.
//

#include "server.h"
#include "db.h"

#include "atomicvar.h"
#include "sds.h"

//#include <signal.h>
//#include <cctype>



void dbAdd(db *db, obj *key, obj *val) {
    sds copy = sdsdup((sds)key->ptr);
    dictAdd(db->dict, copy, val);

//    serverAssertWithInfo(NULL,key,retval == DICT_OK);
//    if (val->type == OBJ_LIST ||
//        val->type == OBJ_ZSET ||
//        val->type == OBJ_STREAM)
//        signalKeyAsReady(db, key);
//    if (server.cluster_enabled) slotToKeyAdd(key->ptr);
}

int dbExists(db *db, obj *key) {
    return dictFind(db->dict,key->ptr) != nullptr;
}

int dbDelete(db *db, obj *key) {
    if (dictDelete(db->dict, key->ptr) == DICT_OK) {
        return 1;
    } else {
        return 0;
    }
}

long long dbTotalServerKeyCount() {
    long long total = 0;
    int j;
    for (j = 0; j < server.dbnum; j++) {
        total += dictSize(server.db[j].dict);
    }
    return total;
}