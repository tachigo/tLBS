//
// Created by liuliwu on 2020-05-11.
//

#include "common.h"
#include "server.h"
#include "db.h"
#include "object.h"

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



/* Db->dict, keys are sds strings, vals are tLBS objects. */
dictType dbDictType = {
        dictSdsHash,                /* hash function */
        nullptr,                       /* key dup */
        nullptr,                       /* val dup */
        dictSdsKeyCompare,          /* key compare */
        dictSdsDestructor,          /* key destructor */
        dictObjectDestructor   /* val destructor */
};

void dbInit() {
    /* Create the databases, and initialize other internal state. */
    for (int j = 0; j < server.dbnum; j++) {
        server.db[j].dict = dictCreate(&dbDictType, nullptr);
//        server.db[j].expires = dictCreate(&keyptrDictType,NULL);
//        server.db[j].expires_cursor = 0;
//        server.db[j].blocking_keys = dictCreate(&keylistDictType,NULL);
//        server.db[j].ready_keys = dictCreate(&objectKeyPointerValueDictType,NULL);
//        server.db[j].watched_keys = dictCreate(&keylistDictType,NULL);
        server.db[j].id = j;
//        server.db[j].avg_ttl = 0;
//        server.db[j].defrag_later = listCreate();
//        listSetFreeMethod(server.db[j].defrag_later,(void (*)(void*))sdsfree);
    }
}