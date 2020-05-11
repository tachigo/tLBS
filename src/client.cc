//
// Created by liuliwu on 2020-05-11.
//

#include "client.h"
#include "server.h"
#include "endianconv.h"
#include "rax.h"
#include "zmalloc.h"

#include <pthread.h>
#include <cstring>

int dbSelect(client *c, int id) {
    if (id < 0 || id >= server.dbnum)
        return C_ERR;
    c->db = &server.db[id];
    return C_OK;
}

void linkClient(client *c) {
    listAddNodeTail(server.clients,c);
    /* Note that we remember the linked list node where the client is stored,
     * this way removing the client in unlinkClient() will not require
     * a linear scan, but just a constant time operation. */
    c->client_list_node = listLast(server.clients);
//    uint64_t id = htonu64(c->id);
//    raxInsert(server.clients_index,(unsigned char*)&id,sizeof(id),c,NULL);
}

client *createClient(connection *conn) {
    auto *c = (client *)zmalloc(sizeof(client));

    /* passing NULL as conn it is possible to create a non connected client.
     * This is useful since all the commands needs to be executed
     * in the context of a client. When commands are executed in other
     * contexts (for instance a Lua script) we need a non connected client. */
    if (conn) {
        connNonBlock(conn);
        connEnableTcpNoDelay(conn);
        if (server.tcpkeepalive)
            connKeepAlive(conn,server.tcpkeepalive);
//        connSetReadHandler(conn, readQueryFromClient);
        connSetPrivateData(conn, c);
    }

//    selectDb(c,0);
    c->db = &server.db[0];
    uint64_t client_id = ++server.next_client_id;
    c->id = client_id;
//    c->resp = 2;
    c->conn = conn;
    c->name = nullptr;
//    c->bufpos = 0;
//    c->qb_pos = 0;
//    c->querybuf = sdsempty();
//    c->pending_querybuf = sdsempty();
//    c->querybuf_peak = 0;
//    c->reqtype = 0;
    c->argc = 0;
    c->argv = nullptr;
//    c->cmd = c->lastcmd = nullptr;
//    c->user = DefaultUser;
//    c->multibulklen = 0;
//    c->bulklen = -1;
//    c->sentlen = 0;
    c->flags = 0;
    c->ctime = c->lastinteraction = server.unixtime;
    /* If the default user does not require authentication, the user is
     * directly authenticated. */
//    c->authenticated = (c->user->flags & USER_FLAG_NOPASS) &&
//                       !(c->user->flags & USER_FLAG_DISABLED);
//    c->replstate = REPL_STATE_NONE;
//    c->repl_put_online_on_ack = 0;
//    c->reploff = 0;
//    c->read_reploff = 0;
//    c->repl_ack_off = 0;
//    c->repl_ack_time = 0;
//    c->slave_listening_port = 0;
//    c->slave_ip[0] = '\0';
//    c->slave_capa = SLAVE_CAPA_NONE;
//    c->reply = listCreate();
//    c->reply_bytes = 0;
//    c->obuf_soft_limit_reached_time = 0;
//    listSetFreeMethod(c->reply,freeClientReplyValue);
//    listSetDupMethod(c->reply,dupClientReplyValue);
//    c->btype = BLOCKED_NONE;
//    c->bpop.timeout = 0;
//    c->bpop.keys = dictCreate(&objectKeyHeapPointerValueDictType,NULL);
//    c->bpop.target = NULL;
//    c->bpop.xread_group = NULL;
//    c->bpop.xread_consumer = NULL;
//    c->bpop.xread_group_noack = 0;
//    c->bpop.numreplicas = 0;
//    c->bpop.reploffset = 0;
//    c->woff = 0;
//    c->watched_keys = listCreate();
//    c->pubsub_channels = dictCreate(&objectKeyPointerValueDictType,NULL);
//    c->pubsub_patterns = listCreate();
//    c->peerid = NULL;
//    c->client_list_node = nullptr;
//    c->client_tracking_redirection = 0;
//    c->client_tracking_prefixes = nullptr;
//    c->client_cron_last_memory_usage = 0;
//    c->client_cron_last_memory_type = CLIENT_TYPE_NORMAL;
//    c->auth_callback = NULL;
//    c->auth_callback_privdata = NULL;
//    c->auth_module = NULL;
//    listSetFreeMethod(c->pubsub_patterns,decrRefCountVoid);
//    listSetMatchMethod(c->pubsub_patterns,listMatchObjects);
    if (conn) linkClient(c);
//    initClientMultiState(c);
    return c;
}

void unlinkClient(client *c) {
    listNode *ln;

    /* If this is marked as current client unset it. */
    if (server.current_client == c) server.current_client = nullptr;

    /* Certain operations must be done only if the client has an active connection.
     * If the client was already unlinked or if it's a "fake client" the
     * conn is already set to NULL. */
    if (c->conn) {
        /* Remove from the list of active clients. */
        if (c->client_list_node) {
            uint64_t id = htonu64(c->id);
            raxRemove(server.clients_index,(unsigned char*)&id,sizeof(id), nullptr);
            listDelNode(server.clients,c->client_list_node);
            c->client_list_node = nullptr;
        }

        /* Check if this is a replica waiting for diskless replication (rdb pipe),
         * in which case it needs to be cleaned from that list */
//        if (c->flags & CLIENT_SLAVE &&
//            c->replstate == SLAVE_STATE_WAIT_BGSAVE_END &&
//            server.rdb_pipe_conns)
//        {
//            int i;
//            for (i=0; i < server.rdb_pipe_numconns; i++) {
//                if (server.rdb_pipe_conns[i] == c->conn) {
//                    rdbPipeWriteHandlerConnRemoved(c->conn);
//                    server.rdb_pipe_conns[i] = NULL;
//                    break;
//                }
//            }
//        }
//        connClose(c->conn);
//        c->conn = NULL;
    }

    /* Remove from the list of pending writes if needed. */
//    if (c->flags & CLIENT_PENDING_WRITE) {
//        ln = listSearchKey(server.clients_pending_write,c);
//        serverAssert(ln != NULL);
//        listDelNode(server.clients_pending_write,ln);
//        c->flags &= ~CLIENT_PENDING_WRITE;
//    }
//
//    /* Remove from the list of pending reads if needed. */
//    if (c->flags & CLIENT_PENDING_READ) {
//        ln = listSearchKey(server.clients_pending_read,c);
//        serverAssert(ln != NULL);
//        listDelNode(server.clients_pending_read,ln);
//        c->flags &= ~CLIENT_PENDING_READ;
//    }

    /* When client was just unblocked because of a blocking operation,
     * remove it from the list of unblocked clients. */
//    if (c->flags & CLIENT_UNBLOCKED) {
//        ln = listSearchKey(server.unblocked_clients,c);
//        serverAssert(ln != NULL);
//        listDelNode(server.unblocked_clients,ln);
//        c->flags &= ~CLIENT_UNBLOCKED;
//    }

    /* Clear the tracking status. */
//    if (c->flags & CLIENT_TRACKING) disableTracking(c);
}

void freeClient(client *c) {
    listNode *ln;

    /* If a client is protected, yet we need to free it right now, make sure
     * to at least use asynchronous freeing. */
//    if (c->flags & CLIENT_PROTECTED) {
//        freeClientAsync(c);
//        return;
//    }

    /* For connected clients, call the disconnection event of modules hooks. */
//    if (c->conn) {
//        moduleFireServerEvent(REDISMODULE_EVENT_CLIENT_CHANGE,
//                              REDISMODULE_SUBEVENT_CLIENT_CHANGE_DISCONNECTED,
//                              c);
//    }

    /* Notify module system that this client auth status changed. */
//    moduleNotifyUserChanged(c);

    /* If it is our master that's beging disconnected we should make sure
     * to cache the state to try a partial resynchronization later.
     *
     * Note that before doing this we make sure that the client is not in
     * some unexpected state, by checking its flags. */
//    if (server.master && c->flags & CLIENT_MASTER) {
//        serverLog(LL_WARNING,"Connection with master lost.");
//        if (!(c->flags & (CLIENT_CLOSE_AFTER_REPLY|
//                          CLIENT_CLOSE_ASAP|
//                          CLIENT_BLOCKED)))
//        {
//            replicationCacheMaster(c);
//            return;
//        }
//    }

    /* Log link disconnection with slave */
//    if (getClientType(c) == CLIENT_TYPE_SLAVE) {
//        serverLog(LL_WARNING,"Connection with replica %s lost.",
//                  replicationGetSlaveName(c));
//    }

    /* Free the query buffer */
//    sdsfree(c->querybuf);
//    sdsfree(c->pending_querybuf);
//    c->querybuf = nullptr;

    /* Deallocate structures used to block on blocking ops. */
//    if (c->flags & CLIENT_BLOCKED) unblockClient(c);
//    dictRelease(c->bpop.keys);

    /* UNWATCH all the keys */
//    unwatchAllKeys(c);
//    listRelease(c->watched_keys);

    /* Unsubscribe from all the pubsub channels */
//    pubsubUnsubscribeAllChannels(c,0);
//    pubsubUnsubscribeAllPatterns(c,0);
//    dictRelease(c->pubsub_channels);
//    listRelease(c->pubsub_patterns);

    /* Free data structures. */
//    listRelease(c->reply);
//    freeClientArgv(c);

    /* Unlink the client: this will close the socket, remove the I/O
     * handlers, and remove references of the client from different
     * places where active clients may be referenced. */
    unlinkClient(c);

    /* Master/slave cleanup Case 1:
     * we lost the connection with a slave. */
//    if (c->flags & CLIENT_SLAVE) {
//        if (c->replstate == SLAVE_STATE_SEND_BULK) {
//            if (c->repldbfd != -1) close(c->repldbfd);
//            if (c->replpreamble) sdsfree(c->replpreamble);
//        }
//        list *l = (c->flags & CLIENT_MONITOR) ? server.monitors : server.slaves;
//        ln = listSearchKey(l,c);
//        serverAssert(ln != NULL);
//        listDelNode(l,ln);
//        /* We need to remember the time when we started to have zero
//         * attached slaves, as after some time we'll free the replication
//         * backlog. */
//        if (getClientType(c) == CLIENT_TYPE_SLAVE && listLength(server.slaves) == 0)
//            server.repl_no_slaves_since = server.unixtime;
//        refreshGoodSlavesCount();
//        /* Fire the replica change modules event. */
//        if (c->replstate == SLAVE_STATE_ONLINE)
//            moduleFireServerEvent(REDISMODULE_EVENT_REPLICA_CHANGE,
//                                  REDISMODULE_SUBEVENT_REPLICA_CHANGE_OFFLINE,
//                                  NULL);
//    }

    /* Master/slave cleanup Case 2:
     * we lost the connection with the master. */
//    if (c->flags & CLIENT_MASTER) replicationHandleMasterDisconnection();

    /* If this client was scheduled for async freeing we need to remove it
     * from the queue. */
    if (c->flags & CLIENT_CLOSE_ASAP) {
        ln = listSearchKey(server.clients_to_close,c);
//        serverAssert(ln != NULL);
        listDelNode(server.clients_to_close,ln);
    }

    /* Remove the contribution that this client gave to our
     * incrementally computed memory usage. */
//    server.stat_clients_type_memory[c->client_cron_last_memory_type] -=
//            c->client_cron_last_memory_usage;

    /* Release other dynamically allocated client structure fields,
     * and finally release the client structure itself. */
    if (c->name) decrRefCount(c->name);
    zfree(c->argv);
//    freeClientMultiState(c);
//    sdsfree(c->peerid);
    zfree(c);
}

void freeClientAsync(client *c) {
    /* We need to handle concurrent access to the server.clients_to_close list
     * only in the freeClientAsync() function, since it's the only function that
     * may access the list while Redis uses I/O threads. All the other accesses
     * are in the context of the main thread while the other threads are
     * idle. */
    if (c->flags & CLIENT_CLOSE_ASAP || c->flags & CLIENT_LUA) return;
    c->flags |= CLIENT_CLOSE_ASAP;
    if (server.io_threads_num == 1) {
        /* no need to bother with locking if there's just one thread (the main thread) */
        listAddNodeTail(server.clients_to_close,c);
        return;
    }
    static pthread_mutex_t async_free_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&async_free_queue_mutex);
    listAddNodeTail(server.clients_to_close,c);
    pthread_mutex_unlock(&async_free_queue_mutex);
}

static void freeClientArgv(client *c) {
    int j;
    for (j = 0; j < c->argc; j++)
        decrRefCount(c->argv[j]);
    c->argc = 0;
//    c->cmd = NULL;
}


void clientAcceptHandler(connection *conn) {
    auto c = (client *)connGetPrivateData(conn);

    if (connGetState(conn) != CONN_STATE_CONNECTED) {
        serverLog(LL_WARNING,
                  "Error accepting a client connection: %s",
                  connGetLastError(conn));
        freeClientAsync(c);
        return;
    }

    /* If the server is running in protected mode (the default) and there
     * is no password set, nor a specific interface is bound, we don't accept
     * requests from non loopback interfaces. Instead we try to explain the
     * user what to do to fix it if needed. */
    if (
//            server.protected_mode &&
            server.bindaddr_count == 0 &&
            //        DefaultUser->flags & USER_FLAG_NOPASS &&
            !(c->flags & CLIENT_UNIX_SOCKET))
    {
        char cip[NET_IP_STR_LEN+1] = { 0 };
        connPeerToString(conn, cip, sizeof(cip)-1, nullptr);

        if (strcmp(cip,"127.0.0.1") && strcmp(cip,"::1")) {
            const char *err =
                    "-DENIED Redis is running in protected mode because protected "
                    "mode is enabled, no bind address was specified, no "
                    "authentication password is requested to clients. In this mode "
                    "connections are only accepted from the loopback interface. "
                    "If you want to connect from external computers to Redis you "
                    "may adopt one of the following solutions: "
                    "1) Just disable protected mode sending the command "
                    "'CONFIG SET protected-mode no' from the loopback interface "
                    "by connecting to Redis from the same host the server is "
                    "running, however MAKE SURE Redis is not publicly accessible "
                    "from internet if you do so. Use CONFIG REWRITE to make this "
                    "change permanent. "
                    "2) Alternatively you can just disable the protected mode by "
                    "editing the Redis configuration file, and setting the protected "
                    "mode option to 'no', and then restarting the server. "
                    "3) If you started the server manually just for testing, restart "
                    "it with the '--protected-mode no' option. "
                    "4) Setup a bind address or an authentication password. "
                    "NOTE: You only need to do one of the above things in order for "
                    "the server to start accepting connections from the outside.\r\n";
            if (connWrite(c->conn,err,strlen(err)) == -1) {
                /* Nothing to do, Just to avoid the warning... */
            }
            server.stat_rejected_conn++;
            freeClientAsync(c);
            return;
        }
    }

//    server.stat_numconnections++;
//    moduleFireServerEvent(REDISMODULE_EVENT_CLIENT_CHANGE,
//                          REDISMODULE_SUBEVENT_CLIENT_CHANGE_CONNECTED,
//                          c);
}