//
// Created by liuliwu on 2020-05-11.
//

#include "server.h"
#include "db.h"
#include "object.h"

#include "atomicvar.h"
#include "sds.h"

//#include <signal.h>
//#include <cctype>
#include <cstring>




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

uint64_t dictSdsHash(const void *key) {
    return dictGenHashFunction((unsigned char*)key, sdslen((char*)key));
}

int dictSdsKeyCompare(void *privdata, const void *key1,
                      const void *key2)
{
    int l1,l2;
    DICT_NOTUSED(privdata);

    l1 = sdslen((sds)key1);
    l2 = sdslen((sds)key2);
    if (l1 != l2) return 0;
    return memcmp(key1, key2, l1) == 0;
}

void dictSdsDestructor(void *privdata, void *val)
{
    DICT_NOTUSED(privdata);

    sdsfree((sds)val);
}

void dictObjectDestructor(void *privdata, void *val)
{
    DICT_NOTUSED(privdata);

    if (val == nullptr) return; /* Lazy freeing will set value to NULL. */
    decrRefCount((obj *)val);
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