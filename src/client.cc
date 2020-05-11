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
//        if (clientsCronHandleTimeout(c,now)) continue;
//        if (clientsCronResizeQueryBuffer(c)) continue;
//        if (clientsCronTrackExpansiveClients(c)) continue;
//        if (clientsCronTrackClientsMemUsage(c)) continue;
    }
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
            if (server.gopher_enabled &&
                ((c->argc == 1 && ((char*)(c->argv[0]->ptr))[0] == '/') ||
                 c->argc == 0))
            {
                processGopherRequest(c);
                resetClient(c);
                c->flags |= CLIENT_CLOSE_AFTER_REPLY;
                break;
            }
        } else if (c->reqtype == PROTO_REQ_MULTIBULK) {
            if (processMultibulkBuffer(c) != C_OK) break;
        } else {
            serverPanic("Unknown request type");
        }

        /* Multibulk processing could see a <= 0 length. */
        if (c->argc == 0) {
            resetClient(c);
        } else {
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