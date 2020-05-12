//
// Created by liuliwu on 2020-05-11.
//

#ifndef TLBS_CLIENT_H
#define TLBS_CLIENT_H

#include <cctype>
#include <ctime>

#include "common.h"
#include "connection.h"
#include "db.h"
#include "object.h"
#include "adlist.h"
#include "sds.h"

/* Client flags */
#define CLIENT_SLAVE (1<<0)   /* This client is a repliaca */
#define CLIENT_MASTER (1<<1)  /* This client is a master */
#define CLIENT_MONITOR (1<<2) /* This client is a slave monitor, see MONITOR */
#define CLIENT_MULTI (1<<3)   /* This client is in a MULTI context */
#define CLIENT_BLOCKED (1<<4) /* The client is waiting in a blocking operation */
#define CLIENT_DIRTY_CAS (1<<5) /* Watched keys modified. EXEC will fail. */
#define CLIENT_CLOSE_AFTER_REPLY (1<<6) /* Close after writing entire reply. */
#define CLIENT_UNBLOCKED (1<<7) /* This client was unblocked and is stored in
                                  server.unblocked_clients */
#define CLIENT_LUA (1<<8) /* This is a non connected client used by Lua */
#define CLIENT_ASKING (1<<9)     /* Client issued the ASKING command */
#define CLIENT_CLOSE_ASAP (1<<10)/* Close this client ASAP */
#define CLIENT_UNIX_SOCKET (1<<11) /* Client connected via Unix domain socket */
#define CLIENT_DIRTY_EXEC (1<<12)  /* EXEC will fail for errors while queueing */
#define CLIENT_MASTER_FORCE_REPLY (1<<13)  /* Queue replies even if is master */
#define CLIENT_FORCE_AOF (1<<14)   /* Force AOF propagation of current cmd. */
#define CLIENT_FORCE_REPL (1<<15)  /* Force replication of current cmd. */
#define CLIENT_PRE_PSYNC (1<<16)   /* Instance don't understand PSYNC. */
#define CLIENT_READONLY (1<<17)    /* Cluster client is in read-only state. */
#define CLIENT_PUBSUB (1<<18)      /* Client is in Pub/Sub mode. */
#define CLIENT_PREVENT_AOF_PROP (1<<19)  /* Don't propagate to AOF. */
#define CLIENT_PREVENT_REPL_PROP (1<<20)  /* Don't propagate to slaves. */
#define CLIENT_PREVENT_PROP (CLIENT_PREVENT_AOF_PROP|CLIENT_PREVENT_REPL_PROP)
#define CLIENT_PENDING_WRITE (1<<21) /* Client has output to send but a write
                                        handler is yet not installed. */
#define CLIENT_REPLY_OFF (1<<22)   /* Don't send replies to client. */
#define CLIENT_REPLY_SKIP_NEXT (1<<23)  /* Set CLIENT_REPLY_SKIP for next cmd */
#define CLIENT_REPLY_SKIP (1<<24)  /* Don't send just this reply. */
#define CLIENT_LUA_DEBUG (1<<25)  /* Run EVAL in debug mode. */
#define CLIENT_LUA_DEBUG_SYNC (1<<26)  /* EVAL debugging without fork() */
#define CLIENT_MODULE (1<<27) /* Non connected client used by some module. */
#define CLIENT_PROTECTED (1<<28) /* Client should not be freed for now. */
#define CLIENT_PENDING_READ (1<<29) /* The client has pending reads and was put
                                       in the list of clients we can read
                                       from. */
#define CLIENT_PENDING_COMMAND (1<<30) /* Used in threaded I/O to signal after
                                          we return single threaded that the
                                          client has already pending commands
                                          to be executed. */
#define CLIENT_TRACKING (1ULL<<31) /* Client enabled keys tracking in order to
                                   perform client side caching. */
#define CLIENT_TRACKING_BROKEN_REDIR (1ULL<<32) /* Target client is invalid. */
#define CLIENT_TRACKING_BCAST (1ULL<<33) /* Tracking in BCAST mode. */
#define CLIENT_TRACKING_OPTIN (1ULL<<34)  /* Tracking in opt-in mode. */
#define CLIENT_TRACKING_OPTOUT (1ULL<<35) /* Tracking in opt-out mode. */
#define CLIENT_TRACKING_CACHING (1ULL<<36) /* CACHING yes/no was given,
                                              depending on optin/optout mode. */
#define CLIENT_TRACKING_NOLOOP (1ULL<<37) /* Don't send invalidation messages
                                             about writes performed by myself.*/
#define CLIENT_IN_TO_TABLE (1ULL<<38) /* This client is in the timeout table. */

/* Client block type (btype field in client structure)
 * if CLIENT_BLOCKED flag is set. */
#define BLOCKED_NONE 0    /* Not blocked, no CLIENT_BLOCKED flag set. */
#define BLOCKED_LIST 1    /* BLPOP & co. */
#define BLOCKED_WAIT 2    /* WAIT for synchronous replication. */
#define BLOCKED_MODULE 3  /* Blocked by a loadable module. */
#define BLOCKED_STREAM 4  /* XREAD. */
#define BLOCKED_ZSET 5    /* BZPOP et al. */
#define BLOCKED_NUM 6     /* Number of blocked states. */

/* Client request types */
#define PROTO_REQ_INLINE 1
#define PROTO_REQ_MULTIBULK 2


#define PROTO_IOBUF_LEN         (1024*16)  /* Generic I/O buffer size */
#define PROTO_REPLY_CHUNK_BYTES (16*1024) /* 16k output buffer */
#define PROTO_INLINE_MAX_SIZE   (1024*64) /* Max size of inline reads */
#define PROTO_MBULK_BIG_ARG     (1024*32)
#define PROTO_REPLY_CHUNK_BYTES (16*1024) /* 16k output buffer */

/* Client classes for client limits, currently used only for
 * the max-client-output-buffer limit implementation. */
#define CLIENT_TYPE_NORMAL 0 /* Normal req-reply clients + MONITORs */
#define CLIENT_TYPE_SLAVE 1  /* Slaves. */
#define CLIENT_TYPE_PUBSUB 2 /* Clients subscribed to PubSub channels. */
#define CLIENT_TYPE_MASTER 3 /* Master. */
#define CLIENT_TYPE_COUNT 4  /* Total number of client types. */
#define CLIENT_TYPE_OBUF_COUNT 3 /* Number of clients to expose to output
                                    buffer configuration. Just the first
                                    three: normal, slave, pubsub. */


#define NET_MAX_WRITES_PER_EVENT (1024*64)



typedef struct tLbsClient {
    uint64_t id;            /* Client incremental unique ID. */
    connection *conn;
//    int resp;               /* RESP protocol version. Can be 2 or 3. */
    db *db;            /* Pointer to currently SELECTed DB. */
    obj *name;             /* As set by CLIENT SETNAME. */
    sds querybuf;           /* Buffer we use to accumulate client queries. */
    size_t qb_pos;          /* The position we have read in querybuf. */
//    sds pending_querybuf;
    /* If this client is flagged as master, this buffer
                               represents the yet not applied portion of the
                               replication stream that we are receiving from
                               the master. */
    size_t querybuf_peak;   /* Recent (100ms or more) peak of querybuf size. */
    int argc;               /* Num of arguments of current command. */
    obj **argv;            /* Arguments of current command. */
    struct tLbsCommand *cmd, *lastcmd;  /* Last command executed. */
//    user *user;
    /* User associated with this connection. If the
                               user is set to NULL the connection can do
                               anything (admin). */
    int reqtype;            /* Request protocol type: PROTO_REQ_* */
//    int multibulklen;       /* Number of multi bulk arguments left to read. */
//    long bulklen;           /* Length of bulk argument in multi bulk request. */
    list *reply;            /* List of reply objects to send to the client. */
    unsigned long long reply_bytes; /* Tot bytes of objects in reply list. */
    size_t sentlen;
    /* Amount of bytes already sent in the current
                               buffer or object being sent. */
    time_t ctime;           /* Client creation time. */
    time_t lastinteraction; /* Time of the last interaction, used for timeout */
    time_t obuf_soft_limit_reached_time;
    uint64_t flags;         /* Client flags: CLIENT_* macros. */
    int authenticated;      /* Needed when the default user requires auth. */
//    int replstate;          /* Replication state if this is a slave. */
//    int repl_put_online_on_ack; /* Install slave write handler on first ACK. */
//    int repldbfd;           /* Replication DB file descriptor. */
//    off_t repldboff;        /* Replication DB file offset. */
//    off_t repldbsize;       /* Replication DB file size. */
//    sds replpreamble;       /* Replication DB preamble. */
//    long long read_reploff; /* Read replication offset if this is a master. */
//    long long reploff;      /* Applied replication offset if this is a master. */
//    long long repl_ack_off; /* Replication ack offset, if this is a slave. */
//    long long repl_ack_time;/* Replication ack time, if this is a slave. */
//    long long psync_initial_offset;
    /* FULLRESYNC reply offset other slaves
                                       copying this slave output buffer
                                       should use. */
//    char replid[CONFIG_RUN_ID_SIZE+1]; /* Master replication ID (if master). */
//    int slave_listening_port; /* As configured with: SLAVECONF listening-port */
//    char slave_ip[NET_IP_STR_LEN]; /* Optionally given by REPLCONF ip-address */
//    int slave_capa;         /* Slave capabilities: SLAVE_CAPA_* bitwise OR. */
//    multiState mstate;      /* MULTI/EXEC state */
//    int btype;              /* Type of blocking op if CLIENT_BLOCKED. */
//    blockingState bpop;     /* blocking state */
//    long long woff;         /* Last write global replication offset. */
//    list *watched_keys;     /* Keys WATCHED for MULTI/EXEC CAS */
//    dict *pubsub_channels;  /* channels a client is interested in (SUBSCRIBE) */
//    list *pubsub_patterns;  /* patterns a client is interested in (SUBSCRIBE) */
//    sds peerid;             /* Cached peer ID. */
    listNode *client_list_node; /* list node in client list */
//    RedisModuleUserChangedFunc auth_callback;
    /* Module callback to execute
                                               * when the authenticated user
                                               * changes. */
//    void *auth_callback_privdata;
    /* Private data that is passed when the auth
                                   * changed callback is executed. Opaque for
                                   * tLBS Core. */
//    void *auth_module;
    /* The module that owns the callback, which is used
                             * to disconnect the client if the module is
                             * unloaded for cleanup. Opaque for tLBS Core.*/

    /* If this client is in tracking mode and this field is non zero,
     * invalidation messages for keys fetched by this client will be send to
     * the specified client ID. */
//    uint64_t client_tracking_redirection;
//    rax *client_tracking_prefixes;
    /* A dictionary of prefixes we are already
                                      subscribed to in BCAST mode, in the
                                      context of client side caching. */
    /* In clientsCronTrackClientsMemUsage() we track the memory usage of
     * each client and add it to the sum of all the clients of a given type,
     * however we need to remember what was the old contribution of each
     * client, and in which categoty the client was, in order to remove it
     * before adding it the new value. */
//    uint64_t client_cron_last_memory_usage;
//    int      client_cron_last_memory_type;
    /* Response buffer */
    int bufpos;
    char buf[PROTO_REPLY_CHUNK_BYTES];
} client;


/* This structure is used in order to represent the output buffer of a client,
 * which is actually a linked list of blocks like that, that is: client->reply. */
typedef struct clientReplyBlock {
    size_t size, used;
    char buf[];
} clientReplyBlock;

int dbSelect(client *c, int id);
void linkClient(client *c);
void unlinkClient(client *c);

client *createClient(connection *conn);

void freeClient(client *c);
void freeClientAsync(client *c);
static void freeClientArgv(client *c);
void clientAcceptHandler(connection *conn);
void readQueryFromClient(connection *conn);
//sds catClientInfoString(sds s, client *client);
void processInputBuffer(client *c);
int processInlineBuffer(client *c);
void resetClient(client *c);
int processCommandAndResetClient(client *c);
int processCommand(client *c);
void commandProcessed(client *c);

#define CLIENTS_CRON_MIN_ITERATIONS 5
void clientsCron();
int clientsCronHandleTimeout(client *c, mstime_t now_ms);

void addReplyProto(client *c, const char *s, size_t len);

void addReply(client *c, obj *obj);
int prepareClientToWrite(client *c);
void clientInstallWriteHandler(client *c);
int clientHasPendingReplies(client *c);
void asyncCloseClientOnOutputBufferLimitReached(client *c);
int checkClientOutputBufferLimits(client *c);
unsigned long getClientOutputBufferMemoryUsage(client *c);
int getClientType(client *c);
void *dupClientReplyValue(void *o);
void freeClientReplyValue(void *o);

void addReplyErrorLength(client *c, const char *s, size_t len);
void addReplyError(client *c, const char *err);
void addReplyErrorFormat(client *c, const char *fmt, ...);


void addReplyStatusLength(client *c, const char *s, size_t len);
void addReplyStatus(client *c, const char *status);
void addReplyStatusFormat(client *c, const char *fmt, ...);

void call(client *c, int flags);

int writeToClient(client *c, int handler_installed);
void sendReplyToClient(connection *conn);
void freeClientsInAsyncFreeQueue();

int getLongLongFromObjectOrReply(client *c, obj *o, long long *target, const char *msg);
int getLongFromObjectOrReply(client *c, obj *o, long *target, const char *msg);
int getDoubleFromObjectOrReply(client *c, obj *o, double *target, const char *msg);
int getLongDoubleFromObjectOrReply(client *c, obj *o, long double *target, const char *msg);

#endif //TLBS_CLIENT_H
