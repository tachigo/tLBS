//
// Created by liuliwu on 2020-05-14.
//

#include "childinfo.h"
#include "server.h"
#include "zmalloc.h"
#include <unistd.h>
#include <cstring>

/* Open a child-parent channel used in order to move information about the
 * RDB / AOF saving process from the child to the parent (for instance
 * the amount of copy on write memory used) */
void openChildInfoPipe() {
    if (pipe(server.child_info_pipe) == -1) {
        /* On error our two file descriptors should be still set to -1,
         * but we call anyway cloesChildInfoPipe() since can't hurt. */
        closeChildInfoPipe();
    } else if (anetNonBlock(nullptr,server.child_info_pipe[0]) != ANET_OK) {
        closeChildInfoPipe();
    } else {
        memset(&server.child_info_data,0,sizeof(server.child_info_data));
    }
}

/* Close the pipes opened with openChildInfoPipe(). */
void closeChildInfoPipe() {
    if (server.child_info_pipe[0] != -1 ||
        server.child_info_pipe[1] != -1)
    {
        close(server.child_info_pipe[0]);
        close(server.child_info_pipe[1]);
        server.child_info_pipe[0] = -1;
        server.child_info_pipe[1] = -1;
    }
}

void sendChildCOWInfo(int ptype, const char *pname) {
    size_t private_dirty = zmalloc_get_private_dirty(-1);

    if (private_dirty) {
        serverLog(LL_NOTICE,
                  "%s: %zu MB of memory used by copy-on-write",
                  pname, private_dirty/(1024*1024));
    }

    server.child_info_data.cow_size = private_dirty;
    sendChildInfo(ptype);
}

/* Send COW data to parent. The child should call this function after populating
 * the corresponding fields it want to sent (according to the process type). */
void sendChildInfo(int ptype) {
    if (server.child_info_pipe[1] == -1) return;
    server.child_info_data.magic = CHILD_INFO_MAGIC;
    server.child_info_data.process_type = ptype;
    ssize_t wlen = sizeof(server.child_info_data);
    if (write(server.child_info_pipe[1],&server.child_info_data,wlen) != wlen) {
        /* Nothing to do on error, this will be detected by the other side. */
    }
}

/* Receive COW data from parent. */
void receiveChildInfo() {
    if (server.child_info_pipe[0] == -1) return;
    ssize_t wlen = sizeof(server.child_info_data);
    if (read(server.child_info_pipe[0],&server.child_info_data,wlen) == wlen &&
        server.child_info_data.magic == CHILD_INFO_MAGIC)
    {
//        if (server.child_info_data.process_type == CHILD_INFO_TYPE_RDB) {
//            server.stat_rdb_cow_bytes = server.child_info_data.cow_size;
//        } else if (server.child_info_data.process_type == CHILD_INFO_TYPE_AOF) {
//            server.stat_aof_cow_bytes = server.child_info_data.cow_size;
//        } else if (server.child_info_data.process_type == CHILD_INFO_TYPE_MODULE) {
//            server.stat_module_cow_bytes = server.child_info_data.cow_size;
//        }
    }
}