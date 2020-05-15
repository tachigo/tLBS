//
// Created by liuliwu on 2020-05-11.
//

#include "common.h"
#include "server.h"
#include "db.h"
#include "object.h"
#include "t_string.h"

#include "atomicvar.h"
#include "sds.h"
#include "debug.h"

#include <cstdio>
#include <sys/stat.h>
#include <cstring>
#include <cerrno>
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
    char datadir[256];
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
        // 创建数据保存目录
        snprintf(datadir,sizeof(datadir),"db#%d", j);
        sds datapath = sdscat(sdsdup(server.persistence_root), datadir);
        if (access((const char *)datapath, F_OK)) {
            if (mkdir(datapath, 0755)) {
                serverPanic(strerror(errno));
            }
        }
    }
}


obj *lookupKey(db *db, obj *key, int flags) {
    dictEntry *de = dictFind(db->dict,key->ptr);
    if (de) {
        obj *val = (obj *)dictGetVal(de);

        /* Update the access time for the ageing algorithm.
         * Don't do it if we have a saving child, as this will trigger
         * a copy on write madness. */
//        if (!hasActiveChildProcess() && !(flags & LOOKUP_NOTOUCH)){
//            if (server.maxmemory_policy & MAXMEMORY_FLAG_LFU) {
//                updateLFU(val);
//            } else {
//                val->lru = LRU_CLOCK();
//            }
//        }
        return val;
    } else {
        return nullptr;
    }
}

obj *lookupKeyWrite(db *db, obj *key) {
    return lookupKeyWriteWithFlags(db, key, LOOKUP_NONE);
}

obj *lookupKeyRead(db *db, obj *key) {
    return lookupKeyReadWithFlags(db,key,LOOKUP_NONE);
}

obj *lookupKeyWriteWithFlags(db *db, obj *key, int flags) {
    return lookupKey(db,key,flags);
}

obj *lookupKeyReadWithFlags(db *db, obj *key, int flags) {
    obj *val;

//    if (expireIfNeeded(db,key) == 1) {
//        /* Key expired. If we are in the context of a master, expireIfNeeded()
//         * returns 0 only when the key does not exist at all, so it's safe
//         * to return NULL ASAP. */
//        if (server.masterhost == NULL) {
//            server.stat_keyspace_misses++;
//            notifyKeyspaceEvent(NOTIFY_KEY_MISS, "keymiss", key, db->id);
//            return NULL;
//        }
//
//        /* However if we are in the context of a slave, expireIfNeeded() will
//         * not really try to expire the key, it only returns information
//         * about the "logical" status of the key: key expiring is up to the
//         * master in order to have a consistent view of master's data set.
//         *
//         * However, if the command caller is not the master, and as additional
//         * safety measure, the command invoked is a read-only command, we can
//         * safely return NULL here, and provide a more consistent behavior
//         * to clients accessign expired values in a read-only fashion, that
//         * will say the key as non existing.
//         *
//         * Notably this covers GETs when slaves are used to scale reads. */
//        if (server.current_client &&
//            server.current_client != server.master &&
//            server.current_client->cmd &&
//            server.current_client->cmd->flags & CMD_READONLY)
//        {
//            server.stat_keyspace_misses++;
//            notifyKeyspaceEvent(NOTIFY_KEY_MISS, "keymiss", key, db->id);
//            return NULL;
//        }
//    }
    val = lookupKey(db,key,flags);
    if (val == nullptr) {
        server.stat_keyspace_misses++;
//        notifyKeyspaceEvent(NOTIFY_KEY_MISS, "keymiss", key, db->id);
    }
    else {
        server.stat_keyspace_hits++;
    }
    return val;
}


/* This callback is used by scanGenericCommand in order to collect elements
 * returned by the dictionary iterator into a list. */
void scanCallback(void *privdata, const dictEntry *de) {
    void **pd = (void**) privdata;
    list *keys = (list *)pd[0];
    obj *o = (obj *)pd[1];
    obj *key, *val = nullptr;

    if (o == nullptr) {
        sds sdskey = (sds)dictGetKey(de);
        key = createStringObject(sdskey, sdslen(sdskey));
//    }
//    else if (o->type == OBJ_SET) {
//        sds keysds = (sds)dictGetKey(de);
//        key = createStringObject(keysds,sdslen(keysds));
//    } else if (o->type == OBJ_HASH) {
//        sds sdskey = (sds)dictGetKey(de);
//        sds sdsval = (sds)dictGetVal(de);
//        key = createStringObject(sdskey,sdslen(sdskey));
//        val = createStringObject(sdsval,sdslen(sdsval));
//    }

//    else if (o->type == OBJ_ZSET) {
//        sds sdskey = (sds)dictGetKey(de);
//        key = createStringObject(sdskey,sdslen(sdskey));
//        val = createStringObjectFromLongDouble(*(double*)dictGetVal(de),0);
    }
    else {
        serverPanic("Type not handled in SCAN callback.");
    }

    listAddNodeTail(keys, key);
    if (val) listAddNodeTail(keys, val);
}