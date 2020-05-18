//
// Created by liuliwu on 2020-05-14.
//


#include "persistence.h"
#include "server.h"
#include "childinfo.h"
#include "debug.h"
#include "db.h"
#include "t_string.h"
#include "t_polygon.h"
#include "zmalloc.h"
#include <cerrno>
#include <cstring>
#include <csignal>
#include <cstdio>
#include <iostream>
#include <fstream>

void killPersistenceChild() {
    kill(server.persistence_child_pid,SIGUSR1);
    persistenceRemoveTempFile(server.persistence_child_pid);
    closeChildInfoPipe();
//    updateDictResizePolicy();
}

void persistenceRemoveTempFile(pid_t childpid) {
    const char *tmpfile = getTempFile(childpid);
    unlink(tmpfile);
}

const char *getTempFile(pid_t childpid) {
    char tmpfile[256];
    snprintf(tmpfile,sizeof(tmpfile),"temp-%d.dat", (int) childpid);
    return sdsnew(tmpfile);
}



/* A background saving child (BGSAVE) terminated its work. Handle this.
 * This function covers the case of actual BGSAVEs. */
void backgroundSaveDoneHandlerDisk(int exitcode, int bysignal) {
    if (!bysignal && exitcode == 0) {
        serverLog(LL_NOTICE,
                  "Background saving terminated with success");
        server.dirty = server.dirty - server.dirty_before_bgsave;
        serverLog(LL_NOTICE, "server.dirty=%llu", server.dirty);
        server.lastsave = time(nullptr);
        server.lastbgsave_status = C_OK;
    } else if (!bysignal && exitcode != 0) {
        serverLog(LL_WARNING, "Background saving error");
        server.lastbgsave_status = C_ERR;
    } else {
//        mstime_t latency;

        serverLog(LL_WARNING,
                  "Background saving terminated by signal %d", bysignal);
//        latencyStartMonitor(latency);
        persistenceRemoveTempFile(server.persistence_child_pid);
//        latencyEndMonitor(latency);
//        latencyAddSampleIfNeeded("rdb-unlink-temp-file",latency);
        /* SIGUSR1 is whitelisted, so we have a way to kill a child without
         * tirggering an error condition. */
        if (bysignal != SIGUSR1)
            server.lastbgsave_status = C_ERR;
    }
    server.persistence_child_pid = -1;
    server.persistence_child_type = PERSISTENCE_CHILD_TYPE_NONE;
    server.persistence_save_time_last = time(nullptr)-server.persistence_save_time_start;
    server.persistence_save_time_start = -1;
    /* Possibly there are slaves waiting for a BGSAVE in order to be served
     * (the first stage of SYNC is a bulk transfer of dump.rdb) */
//    updateSlavesWaitingBgsave((!bysignal && exitcode == 0) ? C_OK : C_ERR, PERSISTENCE_CHILD_TYPE_DISK);
}

/* A background saving child (BGSAVE) terminated its work. Handle this.
 * This function covers the case of PERSISTENCE -> Slaves socket transfers for
 * diskless replication. */
void backgroundSaveDoneHandlerSocket(int exitcode, int bysignal) {
    if (!bysignal && exitcode == 0) {
        serverLog(LL_NOTICE,
                  "Background PERSISTENCE transfer terminated with success");
    } else if (!bysignal && exitcode != 0) {
        serverLog(LL_WARNING, "Background transfer error");
    } else {
        serverLog(LL_WARNING,
                  "Background transfer terminated by signal %d", bysignal);
    }
    server.persistence_child_pid = -1;
    server.persistence_child_type = PERSISTENCE_CHILD_TYPE_NONE;
    server.persistence_save_time_start = -1;

//    updateSlavesWaitingBgsave((!bysignal && exitcode == 0) ? C_OK : C_ERR, PERSISTENCE_CHILD_TYPE_SOCKET);
}


void backgroundSaveDoneHandler(int exitcode, int bysignal) {
    switch(server.persistence_child_type) {
        case PERSISTENCE_CHILD_TYPE_DISK:
            backgroundSaveDoneHandlerDisk(exitcode,bysignal);
            break;
        case PERSISTENCE_CHILD_TYPE_SOCKET:
            backgroundSaveDoneHandlerSocket(exitcode,bysignal);
            break;
        default:
            serverPanic("Unknown PERSISTENCE child type.");
            break;
    }
}


int persistenceSaveBackground(char *persistenceDataDir) {
    pid_t childpid;
    if (hasActiveChildProcess()) return C_ERR;

    server.dirty_before_bgsave = server.dirty;
    server.lastbgsave_try = time(nullptr);
    openChildInfoPipe();

    if ((childpid = serverFork()) == 0) {
        int retval;

        /* Child */
        tLbsSetProcTitle("tlbs-persistence-bgsave");
//        retval = rdbSave(filename,rsi);
        retval = persistenceSave(persistenceDataDir);
        if (retval == C_OK) {
            sendChildCOWInfo(CHILD_INFO_TYPE_PERSISTENCE, "PERSISTENCE");
        }
        exitFromChild((retval == C_OK) ? 0 : 1);
    } else {
        /* Parent */
        if (childpid == -1) {
            closeChildInfoPipe();
            server.lastbgsave_status = C_ERR;
            serverLog(LL_WARNING,"Can't save in background: fork: %s",
                      strerror(errno));
            return C_ERR;
        }
        serverLog(LL_NOTICE,"Background saving started by pid %d",childpid);
        server.persistence_save_time_start = time(nullptr);
        server.persistence_child_pid = childpid;
        server.persistence_child_type = PERSISTENCE_CHILD_TYPE_DISK;
        return C_OK;
    }
    return C_OK; /* unreached */
}


int persistenceSaveDb(char *persistenceDataDir, int dbnum) {
//    serverLog(LL_NOTICE, "开始保存库#%d的数据", dbnum);
    db *db = server.db+dbnum;
    dict *ht = db->dict;
    list *keys = listCreate();
    listNode *node, *nextnode;
    long count = 10;

    if (ht) {
        unsigned long cursor = 0;
        void *privdata[2];
        long maxiterations = count*10;
        privdata[0] = keys;
        privdata[1] = nullptr;
        do {
            cursor = dictScan(ht, cursor, scanCallback, nullptr, privdata);
        } while (cursor &&
                 maxiterations-- &&
                 listLength(keys) < (unsigned long)count);
    }
    // 遍历这个keys
    node = listFirst(keys);
    while (node) {
        obj *kobj = (obj *)listNodeValue(node);
        nextnode = listNextNode(node);
        obj *tableObj = lookupKeyRead(db,kobj);
        serverLog(LL_NOTICE, "db#%d[key=%s][type=%s]pid=%d",
                db->id, kobj->ptr, getObjectTypeName(tableObj), (int) getpid());
        switch (tableObj->type) {
            case OBJ_TYPE_POLYGONS: {
                if (persistenceDumpPolygonIndex(persistenceDataDir, dbnum, kobj, tableObj) != C_OK) {
                    return C_ERR;
                }
                break;
            }
            default:
                serverPanic("Unknown object type");
                break;
        }
        node = nextnode;
    }

    while ((node = listFirst(keys)) != nullptr) {
        obj *kobj = (obj *)listNodeValue(node);
        decrRefCount(kobj);
        listDelNode(keys, node);
    }

    listSetFreeMethod(keys,decrRefCountVoid);
    listRelease(keys);
//    serverLog(LL_NOTICE, "结束保存库#%d的数据", dbnum);
    return C_OK;
}

int persistenceLoad(char *persistenceDataDir) {
    const char *filename = sdscat(sdsdup(persistenceDataDir), "db.dat");
    std::ifstream in(filename);
    if (!in) {
        serverLog(LL_WARNING,
                  "Failed opening the persistence file %s "
                  "for saving: %s",
                  filename,
                  strerror(errno));
        serverPanic("加载持久化数据失败");
    }

    std::string line;
    while (getline(in, line)) {
        int *count = (int *)zmalloc(sizeof(int *));
        sds *arr = sdssplitlen(line.c_str(), line.size(), "/", 1, count);
        if (*count != 3) {
            serverPanic("db数据格式错误");
        }
        int dbid = atoi(arr[0]);
        obj *kobj = createStringObject(arr[1], strlen(arr[1]));
        db *db = server.db + dbid;
        int objType = atoi(arr[2]);
        switch (objType) {
            case OBJ_TYPE_POLYGONS: {
                obj *tableObj = createPolygonObject();
                dbAdd(db, kobj, tableObj);
                persistenceLoadPolygonIndex(persistenceDataDir, db->id, kobj, tableObj);
            }
                break;
            default:
                serverPanic("位置的obj类型");
        }
        zfree(count);
    }

    in.close();
    return C_OK;
}


int persistenceSave(char *persistenceDataDir) {
//    serverLog(LL_WARNING, "持久化数据pid=%d", getpid());
    const char *tmpfile = getTempFile(getpid());
    const char *filename = sdscat(sdsdup(persistenceDataDir), "db.dat");
    FILE *fp;
    fp = fopen(tmpfile,"w");
    if (!fp) {
        serverLog(LL_WARNING,
                  "Failed opening the persistence file %s "
                  "for saving: %s",
                  tmpfile,
                  strerror(errno));
        goto werr;
    }
    for (int i = 0; i < server.dbnum; i++) {
        db *db = server.db+i;
        dict *ht = db->dict;
        list *keys = listCreate();
        listNode *node, *nextnode;
        long count = 10;

        if (ht) {
            unsigned long cursor = 0;
            void *privdata[2];
            long maxiterations = count*10;
            privdata[0] = keys;
            privdata[1] = nullptr;
            do {
                cursor = dictScan(ht, cursor, scanCallback, nullptr, privdata);
            } while (cursor &&
                     maxiterations-- &&
                     listLength(keys) < (unsigned long)count);
        }
        // 遍历这个keys
        node = listFirst(keys);
        while (node) {
            obj *kobj = (obj *)listNodeValue(node);
            nextnode = listNextNode(node);
            obj *tableObj = lookupKeyRead(db,kobj);
//            serverLog(LL_NOTICE, "db#%d[key=%s][type=%s]pid=%d",
//                      db->id, kobj->ptr, getObjectTypeName(tableObj), (int) getpid());
            char tmpline[1024];
            snprintf(tmpline,sizeof(tmpline),"%d/%s/%d\n", db->id, kobj->ptr, tableObj->type);
            const char *line = sdsnew(tmpline);
            fputs(line, fp);
            node = nextnode;
        }

        while ((node = listFirst(keys)) != nullptr) {
            obj *kobj = (obj *)listNodeValue(node);
            decrRefCount(kobj);
            listDelNode(keys, node);
        }

        listSetFreeMethod(keys,decrRefCountVoid);
        listRelease(keys);
    }
    /* Make sure data will not remain on the OS's output buffers */
    if (fflush(fp) == EOF) goto werr;
    if (fsync(fileno(fp)) == -1) goto werr;
    if (fclose(fp) == EOF) goto werr;


    /* Use RENAME to make sure the DB file is changed atomically only
     * if the generate DB file is ok. */
    if (rename(tmpfile,filename) == -1) {
        serverLog(LL_WARNING,
                  "Error moving temp Data file %s on the final "
                  "destination %s: %s",
                  tmpfile,
                  filename,
                  strerror(errno));
        unlink(tmpfile);
        return C_ERR;
    }

    // 遍历每个库
    for (int j = 0; j < server.dbnum; j++) {
        if (persistenceSaveDb(persistenceDataDir, j) != C_OK) {
            return C_ERR;
        }
    }
    serverLog(LL_NOTICE,"DB saved on disk");
    server.dirty = 0;
    server.lastsave = time(nullptr);
    server.lastbgsave_status = C_OK;
    return C_OK;

werr:
    fclose(fp);
    unlink(tmpfile);
    serverLog(LL_WARNING,"Write error saving DB on disk: %s", strerror(errno));
    return C_ERR;
}