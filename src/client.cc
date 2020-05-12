//
// Created by liuliwu on 2020-05-11.
//

#include "client.h"
#include "server.h"
#include "endianconv.h"
#include "rax.h"
#include "zmalloc.h"
#include "debug.h"
#include "command.h"

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
    uint64_t id = htonu64(c->id);
    raxInsert(server.clients_index,(unsigned char*)&id,sizeof(id),c, nullptr);
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
        connSetReadHandler(conn, readQueryFromClient);
        connSetPrivateData(conn, c);
    }

    dbSelect(c,0);
    uint64_t client_id = ++server.next_client_id;
    c->id = client_id;
//    c->resp = 2;
    c->conn = conn;
    c->name = nullptr;
    c->bufpos = 0;
    c->qb_pos = 0;
    c->querybuf = sdsempty();
//    c->pending_querybuf = sdsempty();
    c->querybuf_peak = 0;
    c->reqtype = 0;
    c->argc = 0;
    c->argv = nullptr;
    c->cmd = c->lastcmd = nullptr;
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
    c->client_list_node = nullptr;
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
    serverLog(LL_WARNING, "删除client");
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
        connClose(c->conn);
        c->conn = nullptr;
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
    serverLog(LL_WARNING, "同步释放client");
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
    sdsfree(c->querybuf);
//    sdsfree(c->pending_querybuf);
    c->querybuf = nullptr;

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
    freeClientArgv(c);

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
    serverLog(LL_WARNING, "异步释放client");
    /* We need to handle concurrent access to the server.clients_to_close list
     * only in the freeClientAsync() function, since it's the only function that
     * may access the list while tLBS uses I/O threads. All the other accesses
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
    c->cmd = nullptr;
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
                    "-DENIED tLBS is running in protected mode because protected "
                    "mode is enabled, no bind address was specified, no "
                    "authentication password is requested to clients. In this mode "
                    "connections are only accepted from the loopback interface. "
                    "If you want to connect from external computers to tLBS you "
                    "may adopt one of the following solutions: "
                    "1) Just disable protected mode sending the command "
                    "'CONFIG SET protected-mode no' from the loopback interface "
                    "by connecting to tLBS from the same host the server is "
                    "running, however MAKE SURE tLBS is not publicly accessible "
                    "from internet if you do so. Use CONFIG REWRITE to make this "
                    "change permanent. "
                    "2) Alternatively you can just disable the protected mode by "
                    "editing the tLBS configuration file, and setting the protected "
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

void clientsCron() {
    /* Try to process at least numclients/server.hz of clients
     * per call. Since normally (if there are no big latency events) this
     * function is called server.hz times per second, in the average case we
     * process all the clients in 1 second. */
    int numclients = listLength(server.clients);
    int iterations = numclients/server.hz;
    mstime_t now = mstime();

    /* Process at least a few clients while we are at it, even if we need
     * to process less than CLIENTS_CRON_MIN_ITERATIONS to meet our contract
     * of processing each client once per second. */
    if (iterations < CLIENTS_CRON_MIN_ITERATIONS)
        iterations = (numclients < CLIENTS_CRON_MIN_ITERATIONS) ?
                     numclients : CLIENTS_CRON_MIN_ITERATIONS;

    while(listLength(server.clients) && iterations--) {
        client *c;
        listNode *head;

        /* Rotate the list, take the current head, process.
         * This way if the client must be removed from the list it's the
         * first element and we don't incur into O(N) computation. */
        listRotateTailToHead(server.clients);
        head = listFirst(server.clients);
        c = (client *)listNodeValue(head);
        /* The following functions do different service checks on the client.
         * The protocol is that they return non-zero if the client was
         * terminated. */
        if (clientsCronHandleTimeout(c,now)) continue;
//        if (clientsCronResizeQueryBuffer(c)) continue;
//        if (clientsCronTrackExpansiveClients(c)) continue;
//        if (clientsCronTrackClientsMemUsage(c)) continue;
    }
}

int clientsCronHandleTimeout(client *c, mstime_t now_ms) {
    time_t now = now_ms/1000;

    if (server.maxidletime &&
        /* This handles the idle clients connection timeout if set. */
//        !(c->flags & CLIENT_SLAVE) &&   /* No timeout for slaves and monitors */
//        !(c->flags & CLIENT_MASTER) &&  /* No timeout for masters */
//        !(c->flags & CLIENT_BLOCKED) && /* No timeout for BLPOP */
//        !(c->flags & CLIENT_PUBSUB) &&  /* No timeout for Pub/Sub clients */
        (now - c->lastinteraction > server.maxidletime))
    {
        serverLog(LL_VERBOSE,"Closing idle client");
        freeClient(c);
        return 1;
    }
    else if (c->flags & CLIENT_BLOCKED) {
        /* Cluster: handle unblock & redirect of clients blocked
         * into keys no longer served by this server. */
//        if (server.cluster_enabled) {
//            if (clusterRedirectBlockedClientIfNeeded(c))
//                unblockClient(c);
//        }
    }
    return 0;
}


#define PROTO_IOBUF_LEN         (1024*16)  /* Generic I/O buffer size */
#define PROTO_REPLY_CHUNK_BYTES (16*1024) /* 16k output buffer */
#define PROTO_INLINE_MAX_SIZE   (1024*64) /* Max size of inline reads */
#define PROTO_MBULK_BIG_ARG     (1024*32)
#define LONG_STR_SIZE      21          /* Bytes needed for long -> str + '\0' */

void readQueryFromClient(connection *conn) {
    serverLog(LL_WARNING, "read query from client!");

    auto *c = (client *)connGetPrivateData(conn);
    int nread, readlen;
    size_t qblen;

    /* Check if we want to read from the client later when exiting from
     * the event loop. This is the case if threaded I/O is enabled. */
//    if (postponeClientRead(c)) return;

    readlen = PROTO_IOBUF_LEN;
    /* If this is a multi bulk request, and we are processing a bulk reply
     * that is large enough, try to maximize the probability that the query
     * buffer contains exactly the SDS string representing the object, even
     * at the risk of requiring more read(2) calls. This way the function
     * processMultiBulkBuffer() can avoid copying buffers to create the
     * Redis Object representing the argument. */
//    if (c->reqtype == PROTO_REQ_MULTIBULK && c->multibulklen && c->bulklen != -1
//        && c->bulklen >= PROTO_MBULK_BIG_ARG)
//    {
//        ssize_t remaining = (size_t)(c->bulklen+2)-sdslen(c->querybuf);
//
//        /* Note that the 'remaining' variable may be zero in some edge case,
//         * for example once we resume a blocked client after CLIENT PAUSE. */
//        if (remaining > 0 && remaining < readlen) readlen = remaining;
//    }

    qblen = sdslen(c->querybuf);
    if (c->querybuf_peak < qblen) c->querybuf_peak = qblen;
    c->querybuf = sdsMakeRoomFor(c->querybuf, readlen);
    nread = connRead(c->conn, c->querybuf+qblen, readlen);
    if (nread == -1) {
        if (connGetState(conn) == CONN_STATE_CONNECTED) {
            return;
        } else {
            serverLog(LL_VERBOSE, "Reading from client: %s",connGetLastError(c->conn));
            freeClientAsync(c);
            return;
        }
    }
    else if (nread == 0) {
        serverLog(LL_VERBOSE, "Client closed connection");
        freeClientAsync(c);
        return;
    }
//    else if (c->flags & CLIENT_MASTER) {
//        /* Append the query buffer to the pending (not applied) buffer
//         * of the master. We'll use this buffer later in order to have a
//         * copy of the string applied by the last command executed. */
//        c->pending_querybuf = sdscatlen(c->pending_querybuf,
//                                        c->querybuf+qblen,nread);
//    }

    sdsIncrLen(c->querybuf,nread);
    c->lastinteraction = server.unixtime;
//    if (c->flags & CLIENT_MASTER) c->read_reploff += nread;
//    server.stat_net_input_bytes += nread;
    if (sdslen(c->querybuf) > server.client_max_querybuf_len) {
//        sds ci = catClientInfoString(sdsempty(),c), bytes = sdsempty();
//
//        bytes = sdscatrepr(bytes,c->querybuf,64);
//        serverLog(LL_WARNING,"Closing client that reached max query buffer length: %s (qbuf initial bytes: %s)", ci, bytes);
//        sdsfree(ci);
//        sdsfree(bytes);
        freeClientAsync(c);
        return;
    }

    /* There is more data in the client input buffer, continue parsing it
     * in case to check if there is a full command to execute. */
    processInputBuffer(c);
}

/* This function is called every time, in the client structure 'c', there is
 * more query buffer to process, because we read more data from the socket
 * or because a client was blocked and later reactivated, so there could be
 * pending query buffer, already representing a full command, to process. */
void processInputBuffer(client *c) {
    /* Keep processing while there is something in the input buffer */
    while(c->qb_pos < sdslen(c->querybuf)) {
        /* Return if clients are paused. */
//        if (!(c->flags & CLIENT_SLAVE) && clientsArePaused()) break;

        /* Immediately abort if the client is in the middle of something. */
        if (c->flags & CLIENT_BLOCKED) break;

        /* Don't process more buffers from clients that have already pending
         * commands to execute in c->argv. */
//        if (c->flags & CLIENT_PENDING_COMMAND) break;

        /* Don't process input from the master while there is a busy script
         * condition on the slave. We want just to accumulate the replication
         * stream (instead of replying -BUSY like we do with other clients) and
         * later resume the processing. */
//        if (server.lua_timedout && c->flags & CLIENT_MASTER) break;

        /* CLIENT_CLOSE_AFTER_REPLY closes the connection once the reply is
         * written to the client. Make sure to not let the reply grow after
         * this flag has been set (i.e. don't process more commands).
         *
         * The same applies for clients we want to terminate ASAP. */
        if (c->flags & (CLIENT_CLOSE_AFTER_REPLY|CLIENT_CLOSE_ASAP)) break;

        /* Determine request type when unknown. */
        if (!c->reqtype) {
            if (c->querybuf[c->qb_pos] == '*') {
                c->reqtype = PROTO_REQ_MULTIBULK;
            } else {
                c->reqtype = PROTO_REQ_INLINE;
            }
        }

        if (c->reqtype == PROTO_REQ_INLINE) {
            if (processInlineBuffer(c) != C_OK) break;
            /* If the Gopher mode and we got zero or one argument, process
             * the request in Gopher mode. */
//            if (server.gopher_enabled &&
//                ((c->argc == 1 && ((char*)(c->argv[0]->ptr))[0] == '/') ||
//                 c->argc == 0))
//            {
//                processGopherRequest(c);
//                resetClient(c);
//                c->flags |= CLIENT_CLOSE_AFTER_REPLY;
//                break;
//            }
        }
//        else if (c->reqtype == PROTO_REQ_MULTIBULK) {
//            if (processMultibulkBuffer(c) != C_OK) break;
//        }
        else {
            serverPanic("Unknown request type");
        }

        /* Multibulk processing could see a <= 0 length. */
        if (c->argc == 0) {
            resetClient(c);
        }
        else {
            /* If we are in the context of an I/O thread, we can't really
             * execute the command here. All we can do is to flag the client
             * as one that needs to process the command. */
            if (c->flags & CLIENT_PENDING_READ) {
                c->flags |= CLIENT_PENDING_COMMAND;
                break;
            }

            /* We are finally ready to execute the command. */
            if (processCommandAndResetClient(c) == C_ERR) {
                /* If the client is no longer valid, we avoid exiting this
                 * loop and trimming the client buffer later. So we return
                 * ASAP in that case. */
                return;
            }
        }
    }

    /* Trim to pos */
    if (c->qb_pos) {
        sdsrange(c->querybuf,c->qb_pos,-1);
        c->qb_pos = 0;
    }
}

int processCommandAndResetClient(client *c) {
    int deadclient = 0;
    server.current_client = c;
    serverLog(LL_WARNING, "processCommandAndResetClient");
    if (processCommand(c) == C_OK) {
        commandProcessed(c);
    }
    if (server.current_client == nullptr) deadclient = 1;
    server.current_client = nullptr;
    /* freeMemoryIfNeeded may flush slave output buffers. This may
     * result into a slave, that may be the active client, to be
     * freed. */
    return deadclient ? C_ERR : C_OK;
}

int processCommand(client *c) {
//    moduleCallCommandFilters(c);

    /* The QUIT command is handled separately. Normal command procs will
     * go through checking for replication and QUIT will cause trouble
     * when FORCE_REPLICATION is enabled and would be implemented in
     * a regular command proc. */
    if (!strcasecmp((const char *)c->argv[0]->ptr,"quit")) {
//        addReply(c,shared.ok);
        c->flags |= CLIENT_CLOSE_AFTER_REPLY;
        freeClientAsync(c);
        return C_ERR;
    }

    /* Now lookup the command and check ASAP about trivial error conditions
     * such as wrong arity, bad command name and so forth. */
    c->cmd = c->lastcmd = lookupCommand((sds)c->argv[0]->ptr);
    if (!c->cmd) {
//        flagTransaction(c);
        sds args = sdsempty();
//        int i;
//        for (i=1; i < c->argc && sdslen(args) < 128; i++)
//            args = sdscatprintf(args, "`%.*s`, ", 128-(int)sdslen(args), (char*)c->argv[i]->ptr);
//        addReplyErrorFormat(c,"unknown command `%s`, with args beginning with: %s",
//                            (char*)c->argv[0]->ptr, args);
        serverLog(LL_WARNING,
                "unknown command `%s`, with args beginning with: %s",
                (char*)c->argv[0]->ptr, args);
        sdsfree(args);
        return C_OK;
    } else if ((c->cmd->arity > 0 && c->cmd->arity != c->argc) ||
               (c->argc < -c->cmd->arity)) {
//        flagTransaction(c);
//        addReplyErrorFormat(c,"wrong number of arguments for '%s' command",
//                            c->cmd->name);
        serverLog(LL_WARNING, "wrong number of arguments for '%s' command",
                            c->cmd->name);
        return C_OK;
    }

    /* Check if the user is authenticated. This check is skipped in case
     * the default user is flagged as "nopass" and is active. */
//    int auth_required = (!(DefaultUser->flags & USER_FLAG_NOPASS) ||
//                         (DefaultUser->flags & USER_FLAG_DISABLED)) &&
//                        !c->authenticated;
//    if (auth_required) {
//        /* AUTH and HELLO and no auth modules are valid even in
//         * non-authenticated state. */
//        if (!(c->cmd->flags & CMD_NO_AUTH)) {
//            flagTransaction(c);
//            addReply(c,shared.noautherr);
//            return C_OK;
//        }
//    }

    /* Check if the user can run this command according to the current
     * ACLs. */
//    int acl_keypos;
//    int acl_retval = ACLCheckCommandPerm(c,&acl_keypos);
//    if (acl_retval != ACL_OK) {
//        addACLLogEntry(c,acl_retval,acl_keypos,NULL);
//        flagTransaction(c);
//        if (acl_retval == ACL_DENIED_CMD)
//            addReplyErrorFormat(c,
//                                "-NOPERM this user has no permissions to run "
//                                "the '%s' command or its subcommand", c->cmd->name);
//        else
//            addReplyErrorFormat(c,
//                                "-NOPERM this user has no permissions to access "
//                                "one of the keys used as arguments");
//        return C_OK;
//    }

    /* If cluster is enabled perform the cluster redirection here.
     * However we don't perform the redirection if:
     * 1) The sender of this command is our master.
     * 2) The command has no key arguments. */
//    if (server.cluster_enabled &&
//        !(c->flags & CLIENT_MASTER) &&
//        !(c->flags & CLIENT_LUA &&
//          server.lua_caller->flags & CLIENT_MASTER) &&
//        !(c->cmd->getkeys_proc == NULL && c->cmd->firstkey == 0 &&
//          c->cmd->proc != execCommand))
//    {
//        int hashslot;
//        int error_code;
//        clusterNode *n = getNodeByQuery(c,c->cmd,c->argv,c->argc,
//                                        &hashslot,&error_code);
//        if (n == NULL || n != server.cluster->myself) {
//            if (c->cmd->proc == execCommand) {
//                discardTransaction(c);
//            } else {
//                flagTransaction(c);
//            }
//            clusterRedirectClient(c,n,hashslot,error_code);
//            return C_OK;
//        }
//    }

    /* Handle the maxmemory directive.
     *
     * Note that we do not want to reclaim memory if we are here re-entering
     * the event loop since there is a busy Lua script running in timeout
     * condition, to avoid mixing the propagation of scripts with the
     * propagation of DELs due to eviction. */
//    if (server.maxmemory && !server.lua_timedout) {
//        int out_of_memory = freeMemoryIfNeededAndSafe() == C_ERR;
//        /* freeMemoryIfNeeded may flush slave output buffers. This may result
//         * into a slave, that may be the active client, to be freed. */
//        if (server.current_client == NULL) return C_ERR;
//
//        /* It was impossible to free enough memory, and the command the client
//         * is trying to execute is denied during OOM conditions or the client
//         * is in MULTI/EXEC context? Error. */
//        if (out_of_memory &&
//            (c->cmd->flags & CMD_DENYOOM ||
//             (c->flags & CLIENT_MULTI &&
//              c->cmd->proc != execCommand &&
//              c->cmd->proc != discardCommand)))
//        {
//            flagTransaction(c);
//            addReply(c, shared.oomerr);
//            return C_OK;
//        }
//
//        /* Save out_of_memory result at script start, otherwise if we check OOM
//         * untill first write within script, memory used by lua stack and
//         * arguments might interfere. */
//        if (c->cmd->proc == evalCommand || c->cmd->proc == evalShaCommand) {
//            server.lua_oom = out_of_memory;
//        }
//    }

    /* Make sure to use a reasonable amount of memory for client side
     * caching metadata. */
//    if (server.tracking_clients) trackingLimitUsedSlots();

    /* Don't accept write commands if there are problems persisting on disk
     * and if this is a master instance. */
//    int deny_write_type = writeCommandsDeniedByDiskError();
//    if (deny_write_type != DISK_ERROR_TYPE_NONE &&
//        server.masterhost == NULL &&
//        (c->cmd->flags & CMD_WRITE ||
//         c->cmd->proc == pingCommand))
//    {
//        flagTransaction(c);
//        if (deny_write_type == DISK_ERROR_TYPE_RDB)
//            addReply(c, shared.bgsaveerr);
//        else
//            addReplySds(c,
//                        sdscatprintf(sdsempty(),
//                                     "-MISCONF Errors writing to the AOF file: %s\r\n",
//                                     strerror(server.aof_last_write_errno)));
//        return C_OK;
//    }

    /* Don't accept write commands if there are not enough good slaves and
     * user configured the min-slaves-to-write option. */
//    if (server.masterhost == nullptr &&
//        server.repl_min_slaves_to_write &&
//        server.repl_min_slaves_max_lag &&
//        c->cmd->flags & CMD_WRITE &&
//        server.repl_good_slaves_count < server.repl_min_slaves_to_write)
//    {
//        flagTransaction(c);
//        addReply(c, shared.noreplicaserr);
//        return C_OK;
//    }

    /* Don't accept write commands if this is a read only slave. But
     * accept write commands if this is our master. */
//    if (server.masterhost && server.repl_slave_ro &&
//        !(c->flags & CLIENT_MASTER) &&
//        c->cmd->flags & CMD_WRITE)
//    {
//        flagTransaction(c);
//        addReply(c, shared.roslaveerr);
//        return C_OK;
//    }

    /* Only allow a subset of commands in the context of Pub/Sub if the
     * connection is in RESP2 mode. With RESP3 there are no limits. */
//    if ((c->flags & CLIENT_PUBSUB && c->resp == 2) &&
//        c->cmd->proc != pingCommand &&
//        c->cmd->proc != subscribeCommand &&
//        c->cmd->proc != unsubscribeCommand &&
//        c->cmd->proc != psubscribeCommand &&
//        c->cmd->proc != punsubscribeCommand) {
//        addReplyErrorFormat(c,
//                            "Can't execute '%s': only (P)SUBSCRIBE / "
//                            "(P)UNSUBSCRIBE / PING / QUIT are allowed in this context",
//                            c->cmd->name);
//        return C_OK;
//    }

    /* Only allow commands with flag "t", such as INFO, SLAVEOF and so on,
     * when slave-serve-stale-data is no and we are a slave with a broken
     * link with master. */
//    if (server.masterhost && server.repl_state != REPL_STATE_CONNECTED &&
//        server.repl_serve_stale_data == 0 &&
//        !(c->cmd->flags & CMD_STALE))
//    {
//        flagTransaction(c);
//        addReply(c, shared.masterdownerr);
//        return C_OK;
//    }

    /* Loading DB? Return an error if the command has not the
     * CMD_LOADING flag. */
//    if (server.loading && !(c->cmd->flags & CMD_LOADING)) {
//        addReply(c, shared.loadingerr);
//        return C_OK;
//    }

    /* Lua script too slow? Only allow a limited number of commands.
     * Note that we need to allow the transactions commands, otherwise clients
     * sending a transaction with pipelining without error checking, may have
     * the MULTI plus a few initial commands refused, then the timeout
     * condition resolves, and the bottom-half of the transaction gets
     * executed, see Github PR #7022. */
//    if (server.lua_timedout &&
//        c->cmd->proc != authCommand &&
//        c->cmd->proc != helloCommand &&
//        c->cmd->proc != replconfCommand &&
//        c->cmd->proc != multiCommand &&
//        c->cmd->proc != execCommand &&
//        c->cmd->proc != discardCommand &&
//        !(c->cmd->proc == shutdownCommand &&
//          c->argc == 2 &&
//          tolower(((char*)c->argv[1]->ptr)[0]) == 'n') &&
//        !(c->cmd->proc == scriptCommand &&
//          c->argc == 2 &&
//          tolower(((char*)c->argv[1]->ptr)[0]) == 'k'))
//    {
//        flagTransaction(c);
//        addReply(c, shared.slowscripterr);
//        return C_OK;
//    }

    /* Exec the command */
//    if (c->flags & CLIENT_MULTI &&
//        c->cmd->proc != execCommand && c->cmd->proc != discardCommand &&
//        c->cmd->proc != multiCommand && c->cmd->proc != watchCommand)
//    {
//        queueMultiCommand(c);
//        addReply(c,shared.queued);
//    } else {
    serverLog(LL_WARNING, "execute command: `%s`",  c->cmd->name);
//        call(c,CMD_CALL_FULL);
//        c->woff = server.master_repl_offset;
//        if (listLength(server.ready_keys))
//            handleClientsBlockedOnKeys();
//    }
    return C_OK;
}

void commandProcessed(client *c) {
//    int cmd_is_ping = c->cmd && c->cmd->proc == pingCommand;
//    long long prev_offset = c->reploff;
//    if (c->flags & CLIENT_MASTER && !(c->flags & CLIENT_MULTI)) {
//        /* Update the applied replication offset of our master. */
//        c->reploff = c->read_reploff - sdslen(c->querybuf) + c->qb_pos;
//    }

    /* Don't reset the client structure for clients blocked in a
     * module blocking command, so that the reply callback will
     * still be able to access the client argv and argc field.
     * The client will be reset in unblockClientFromModule(). */
//    if (!(c->flags & CLIENT_BLOCKED) ||
//        c->btype != BLOCKED_MODULE)
//    {
//        resetClient(c);
//    }

    /* If the client is a master we need to compute the difference
     * between the applied offset before and after processing the buffer,
     * to understand how much of the replication stream was actually
     * applied to the master state: this quantity, and its corresponding
     * part of the replication stream, will be propagated to the
     * sub-replicas and to the replication backlog. */
//    if (c->flags & CLIENT_MASTER) {
//        long long applied = c->reploff - prev_offset;
//        long long prev_master_repl_meaningful_offset = server.master_repl_meaningful_offset;
//        if (applied) {
//            replicationFeedSlavesFromMasterStream(server.slaves,
//                                                  c->pending_querybuf, applied);
//            sdsrange(c->pending_querybuf,applied,-1);
//        }
//        /* The server.master_repl_meaningful_offset variable represents
//         * the offset of the replication stream without the pending PINGs. */
//        if (cmd_is_ping)
//            server.master_repl_meaningful_offset = prev_master_repl_meaningful_offset;
//    }
}

int processInlineBuffer(client *c) {
    char *newline;
    int argc, j, linefeed_chars = 1;
    sds *argv, aux;
    size_t querylen;

    /* Search for end of line */
    newline = strchr(c->querybuf+c->qb_pos,'\n');

    /* Nothing to do without a \r\n */
    if (newline == nullptr) {
        if (sdslen(c->querybuf)-c->qb_pos > PROTO_INLINE_MAX_SIZE) {
//            addReplyError(c,"Protocol error: too big inline request");
//            setProtocolError("too big inline request",c);
        }
        return C_ERR;
    }

    /* Handle the \r\n case. */
    if (newline && newline != c->querybuf+c->qb_pos && *(newline-1) == '\r')
        newline--, linefeed_chars++;

    /* Split the input buffer up to the \r\n */
    querylen = newline-(c->querybuf+c->qb_pos);
    aux = sdsnewlen(c->querybuf+c->qb_pos,querylen);
    argv = sdssplitargs(aux,&argc);
    sdsfree(aux);
    if (argv == nullptr) {
//        addReplyError(c,"Protocol error: unbalanced quotes in request");
//        setProtocolError("unbalanced quotes in inline request",c);
        return C_ERR;
    }

    /* Newline from slaves can be used to refresh the last ACK time.
     * This is useful for a slave to ping back while loading a big
     * RDB file. */
//    if (querylen == 0 && getClientType(c) == CLIENT_TYPE_SLAVE)
//        c->repl_ack_time = server.unixtime;

    /* Move querybuffer position to the next query in the buffer. */
    c->qb_pos += querylen+linefeed_chars;

    /* Setup argv array on client structure */
    if (argc) {
        if (c->argv) zfree(c->argv);
        c->argv = (obj **)zmalloc(sizeof(obj*)*argc);
    }

    /* Create redis objects for all arguments. */
    for (c->argc = 0, j = 0; j < argc; j++) {
        c->argv[c->argc] = createObject(OBJ_TYPE_STRING,argv[j]);
        c->argc++;
    }
    zfree(argv);
    return C_OK;
}


void resetClient(client *c) {
//    redisCommandProc *prevcmd = c->cmd ? c->cmd->proc : NULL;

    freeClientArgv(c);
    c->reqtype = 0;
//    c->multibulklen = 0;
//    c->bulklen = -1;

    /* We clear the ASKING flag as well if we are not inside a MULTI, and
     * if what we just executed is not the ASKING command itself. */
//    if (!(c->flags & CLIENT_MULTI) && prevcmd != askingCommand)
//        c->flags &= ~CLIENT_ASKING;

    /* We do the same for the CACHING command as well. It also affects
     * the next command or transaction executed, in a way very similar
     * to ASKING. */
//    if (!(c->flags & CLIENT_MULTI) && prevcmd != clientCommand)
//        c->flags &= ~CLIENT_TRACKING_CACHING;

    /* Remove the CLIENT_REPLY_SKIP flag if any so that the reply
     * to the next command will be sent, but set the flag if the command
     * we just processed was "CLIENT REPLY SKIP". */
    c->flags &= ~CLIENT_REPLY_SKIP;
    if (c->flags & CLIENT_REPLY_SKIP_NEXT) {
        c->flags |= CLIENT_REPLY_SKIP;
        c->flags &= ~CLIENT_REPLY_SKIP_NEXT;
    }
}


/* Concatenate a string representing the state of a client in an human
 * readable format, into the sds string 's'. */
//sds catClientInfoString(sds s, client *client) {
//    char flags[16], events[3], conninfo[CONN_INFO_LEN], *p;
//
//    p = flags;
//    if (client->flags & CLIENT_SLAVE) {
//        if (client->flags & CLIENT_MONITOR)
//            *p++ = 'O';
//        else
//            *p++ = 'S';
//    }
//    if (client->flags & CLIENT_MASTER) *p++ = 'M';
//    if (client->flags & CLIENT_PUBSUB) *p++ = 'P';
//    if (client->flags & CLIENT_MULTI) *p++ = 'x';
//    if (client->flags & CLIENT_BLOCKED) *p++ = 'b';
//    if (client->flags & CLIENT_TRACKING) *p++ = 't';
//    if (client->flags & CLIENT_TRACKING_BROKEN_REDIR) *p++ = 'R';
//    if (client->flags & CLIENT_DIRTY_CAS) *p++ = 'd';
//    if (client->flags & CLIENT_CLOSE_AFTER_REPLY) *p++ = 'c';
//    if (client->flags & CLIENT_UNBLOCKED) *p++ = 'u';
//    if (client->flags & CLIENT_CLOSE_ASAP) *p++ = 'A';
//    if (client->flags & CLIENT_UNIX_SOCKET) *p++ = 'U';
//    if (client->flags & CLIENT_READONLY) *p++ = 'r';
//    if (p == flags) *p++ = 'N';
//    *p++ = '\0';
//
//    p = events;
//    if (client->conn) {
//        if (connHasReadHandler(client->conn)) *p++ = 'r';
//        if (connHasWriteHandler(client->conn)) *p++ = 'w';
//    }
//    *p = '\0';
//    return sdscatfmt(s,
//                     "id=%U addr=%s %s name=%s age=%I idle=%I flags=%s db=%i sub=%i psub=%i multi=%i qbuf=%U qbuf-free=%U obl=%U oll=%U omem=%U events=%s cmd=%s user=%s",
//                     (unsigned long long) client->id,
//                     getClientPeerId(client),
//                     connGetInfo(client->conn, conninfo, sizeof(conninfo)),
//                     client->name ? (char*)client->name->ptr : "",
//                     (long long)(server.unixtime - client->ctime),
//                     (long long)(server.unixtime - client->lastinteraction),
//                     flags,
//                     client->db->id,
//                     (int) dictSize(client->pubsub_channels),
//                     (int) listLength(client->pubsub_patterns),
//                     (client->flags & CLIENT_MULTI) ? client->mstate.count : -1,
//                     (unsigned long long) sdslen(client->querybuf),
//                     (unsigned long long) sdsavail(client->querybuf),
//                     (unsigned long long) client->bufpos,
//                     (unsigned long long) listLength(client->reply),
//                     (unsigned long long) getClientOutputBufferMemoryUsage(client),
//                     events,
//                     client->lastcmd ? client->lastcmd->name : "NULL",
//                     client->user ? client->user->name : "(superuser)");
//}