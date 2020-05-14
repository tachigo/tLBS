//
// Created by liuliwu on 2020-05-14.
//


#include "persistence.h"
#include "server.h"
#include "childinfo.h"
#include "debug.h"
#include "db.h"
//#include <sys/types.h>
#include <cerrno>
#include <cstring>
#include <csignal>
#include <cstdio>
#include <sys/param.h>

void killPersistenceChild() {
    kill(server.persistence_child_pid,SIGUSR1);
    persistenceRemoveTempFile(server.persistence_child_pid);
    closeChildInfoPipe();
//    updateDictResizePolicy();
}

void persistenceRemoveTempFile(pid_t childpid) {
    char tmpfile[256];

    snprintf(tmpfile,sizeof(tmpfile),"temp-%d.rdb", (int) childpid);
    unlink(tmpfile);
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
        mstime_t latency;

        serverLog(LL_WARNING,
                  "Background saving terminated by signal %d", bysignal);
//        latencyStartMonitor(latency);
//        rdbRemoveTempFile(server.rdb_child_pid);
//        latencyEndMonitor(latency);
//        latencyAddSampleIfNeeded("rdb-unlink-temp-file",latency);
        /* SIGUSR1 is whitelisted, so we have a way to kill a child without
         * tirggering an error condition. */
        if (bysignal != SIGUSR1)
            server.lastbgsave_status = C_ERR;
    }
    server.persistence_child_pid = -1;
    server.persistence_child_type = PERSISTENCE_CHILD_TYPE_NONE;
    server.persistence_save_time_last = time(NULL)-server.persistence_save_time_start;
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
    serverLog(LL_NOTICE, "开始保存库#%d的数据", dbnum);
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
        sds key = (sds) kobj->ptr;
        obj *tableObj = lookupKeyRead(db,kobj);
        serverLog(LL_NOTICE, "db#%d[key=%s][type=%s]", db->id, key, getObjectTypeName(tableObj));

        node = nextnode;
    }

    while ((node = listFirst(keys)) != nullptr) {
        obj *kobj = (obj *)listNodeValue(node);
        decrRefCount(kobj);
        listDelNode(keys, node);
    }

    listSetFreeMethod(keys,decrRefCountVoid);
    listRelease(keys);

//    char tmpfile[256];
//    char cwd[MAXPATHLEN]; /* Current working dir path for error messages. */
//    FILE *fp;
//    rio rdb;
//    int error = 0;
//
//    snprintf(tmpfile,256,"temp-%d.rdb", (int) getpid());
//    fp = fopen(tmpfile,"w");
//    if (!fp) {
//        char *cwdp = getcwd(cwd,MAXPATHLEN);
//        serverLog(LL_WARNING,
//                  "Failed opening the persistence file %s (in server root dir %s) "
//                  "for saving: %s",
//                  tmpfile,
//                  cwdp ? cwdp : "unknown",
//                  strerror(errno));
//        return C_ERR;
//    }
//
//
//    /* Make sure data will not remain on the OS's output buffers */
//    if (fflush(fp) == EOF) goto werr;
//    if (fsync(fileno(fp)) == -1) goto werr;
//    if (fclose(fp) == EOF) goto werr;
//
//    /* Use RENAME to make sure the DB file is changed atomically only
//     * if the generate DB file is ok. */
//    if (rename(tmpfile,filename) == -1) {
//        char *cwdp = getcwd(cwd,MAXPATHLEN);
//        serverLog(LL_WARNING,
//                  "Error moving temp DB file %s on the final "
//                  "destination %s (in server root dir %s): %s",
//                  tmpfile,
//                  filename,
//                  cwdp ? cwdp : "unknown",
//                  strerror(errno));
//        unlink(tmpfile);
//        stopSaving(0);
//        return C_ERR;
//    }
//
//    serverLog(LL_NOTICE,"DB saved on disk");
//    server.dirty = 0;
//    server.lastsave = time(NULL);
//    server.lastbgsave_status = C_OK;
//    stopSaving(1);
//    return C_OK;
//
//werr:
//    serverLog(LL_WARNING,"Write error saving DB on disk: %s", strerror(errno));
//    fclose(fp);
//    unlink(tmpfile);
//    stopSaving(0);
//    return C_ERR;
    serverLog(LL_NOTICE, "结束保存库#%d的数据", dbnum);
    return C_OK;
}


int persistenceSave(char *persistenceDataDir) {
    serverLog(LL_WARNING, "持久化数据pid=%d", getpid());
    // 遍历每个库
    for (int j = 0; j < server.dbnum; j++) {
        if (persistenceSaveDb(persistenceDataDir, j) != C_OK) {
            return C_ERR;
        }
    }
    return C_OK;
}