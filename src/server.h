//
// Created by liuliwu on 2020-05-08.
//

#ifndef TLBS_SERVER_H
#define TLBS_SERVER_H

#include "common.h"

#include "db.h"
#include "client.h"
#include "command.h"

#include "ae.h"      /* Event driven programming library */
#include "anet.h"
#include "adlist.h"  /* Linked lists */
#include "rax.h"



/* SHUTDOWN flags */
#define SHUTDOWN_NOFLAGS 0      /* No flags. */
#define SHUTDOWN_SAVE 1         /* Force SAVE on SHUTDOWN even if no save
                                   points are configured. */
#define SHUTDOWN_NOSAVE 2       /* Don't SAVE on SHUTDOWN. */

//struct saveparam {
//    time_t seconds;
//    int changes;
//};

#define CONFIG_MAX_LINE    1024
#define CONFIG_RUN_ID_SIZE 40
#define CONFIG_DEFAULT_LOGFILE ""
#define CONFIG_BINDADDR_MAX 16
#define CONFIG_DEFAULT_PID_FILE "/var/run/tlbs.pid"


#define CONFIG_DEFAULT_HZ        10             /* Time interrupt calls/sec. */
#define CONFIG_MIN_HZ            1
#define CONFIG_MAX_HZ            500
#define MAX_CLIENTS_PER_CLOCK_TICK 200          /* HZ is adapted based on that. */

/* Client block type (btype field in client structure)
 * if CLIENT_BLOCKED flag is set. */
#define BLOCKED_NONE 0    /* Not blocked, no CLIENT_BLOCKED flag set. */
//#define BLOCKED_LIST 1    /* BLPOP & co. */
//#define BLOCKED_WAIT 2    /* WAIT for synchronous replication. */
//#define BLOCKED_MODULE 3  /* Blocked by a loadable module. */
//#define BLOCKED_STREAM 4  /* XREAD. */
//#define BLOCKED_ZSET 5    /* BZPOP et al. */
#define BLOCKED_NUM 6     /* Number of blocked states. */

#define CONFIG_MIN_RESERVED_FDS 32
/* When configuring the server eventloop, we setup it so that the total number
 * of file descriptors we can handle are server.maxclients + RESERVED_FDS +
 * a few more to stay safe. Since RESERVED_FDS defaults to 32, we add 96
 * in order to make sure of not over provisioning more than 128 fds. */
#define CONFIG_FDSET_INCR (CONFIG_MIN_RESERVED_FDS+96)
#define NET_IP_STR_LEN 46 /* INET6_ADDRSTRLEN is 46, but we need to be sure */

#define PROTO_MAX_QUERYBUF_LEN  (1024*1024*1024) /* 1GB max query buffer. */

#define MAXMEMORY_NO_EVICTION (7<<8)

struct tLbsServer {
    /* General */
    pid_t pid;                  /* Main process pid. */
    char *configfile;           /* Absolute config file path, or NULL */
    char *executable;           /* Absolute executable file path. */
    char **exec_argv;           /* Executable argv vector (copy). */
    int dynamic_hz;             /* Change hz value depending on # of clients. */
    int config_hz;              /* Configured HZ value. May be different than
                                   the actual 'hz' field value if dynamic-hz
                                   is enabled. */
    int hz;                     /* serverCron() calls frequency in hertz */
    db *db;
    dict *commands;             /* Command table */
    dict *orig_commands;        /* Command table before command renaming. */
    aeEventLoop *el;
//    _Atomic unsigned int lruclock; /* Clock for LRU eviction */
    int shutdown_asap;          /* SHUTDOWN needed ASAP */
//    int activerehashing;        /* Incremental rehash in serverCron() */
//    int active_defrag_running;  /* Active defragmentation running (holds current scan aggressiveness) */
    char *pidfile;              /* PID file path */
    int arch_bits;              /* 32 or 64 depending on sizeof(long) */
    int cronloops;              /* Number of times the cron function run */
    char runid[CONFIG_RUN_ID_SIZE+1];  /* ID always different at every exec. */
    int sentinel_mode;          /* True if this instance is a Sentinel. */
//    size_t initial_memory_usage; /* Bytes used after initialization. */
//    int always_show_logo;       /* Show logo even for non-stdout logging. */
    /* Modules */
//    dict *moduleapi;            /* Exported core APIs dictionary for modules. */
//    dict *sharedapi;
                                  /* Like moduleapi but containing the APIs that
                                   modules share with each other. */
//    list *loadmodule_queue;     /* List of modules to load at startup. */
//    int module_blocked_pipe[2];
                                /* Pipe used to awake the event loop if a
                                   client blocked on a module command needs
                                   to be processed. */
//    pid_t module_child_pid;     /* PID of module child */
    /* Networking */
    int port;                   /* TCP listening port */
//    int tls_port;               /* TLS listening port */
    int tcp_backlog;            /* TCP listen() backlog */
    char *bindaddr[CONFIG_BINDADDR_MAX]; /* Addresses we should bind to */
    int bindaddr_count;         /* Number of addresses in server.bindaddr[] */
//    char *unixsocket;           /* UNIX socket path */
//    mode_t unixsocketperm;      /* UNIX socket permission */
    int ipfd[CONFIG_BINDADDR_MAX]; /* TCP socket file descriptors */
    int ipfd_count;             /* Used slots in ipfd[] */
//    int tlsfd[CONFIG_BINDADDR_MAX]; /* TLS socket file descriptors */
//    int tlsfd_count;            /* Used slots in tlsfd[] */
//    int sofd;                   /* Unix socket file descriptor */
//    int cfd[CONFIG_BINDADDR_MAX];/* Cluster bus listening socket */
//    int cfd_count;              /* Used slots in cfd[] */
    list *clients;              /* List of active clients */
    list *clients_to_close;     /* Clients to close asynchronously */
    list *clients_pending_write; /* There is to write or install handler. */
    list *clients_pending_read;  /* Client has pending read socket buffers. */
    list *slaves, *monitors;    /* List of slaves and MONITORs */
    client *current_client;     /* Current client executing the command. */
    rax *clients_timeout_table; /* Radix tree for blocked clients timeouts. */
    long fixed_time_expire;     /* If > 0, expire keys against server.mstime. */
    rax *clients_index;         /* Active clients dictionary by client ID. */
    int clients_paused;         /* True if clients are currently paused */
    mstime_t clients_pause_end_time; /* Time when we undo clients_paused */
    char neterr[ANET_ERR_LEN];   /* Error buffer for anet.c */
//    dict *migrate_cached_sockets;/* MIGRATE cached sockets */
    _Atomic uint64_t next_client_id; /* Next client unique ID. Incremental. */
//    int protected_mode;         /* Don't accept external connections. */
//    int gopher_enabled;
    /* If true the server will reply to gopher
                                   queries. Will still serve RESP2 queries. */
    int io_threads_num;         /* Number of IO threads to use. */
//    int io_threads_do_reads;    /* Read and parse from IO threads? */

    /* RDB / AOF loading information */
    int loading;                /* We are loading data from disk if true */
//    off_t loading_total_bytes;
//    off_t loading_loaded_bytes;
//    time_t loading_start_time;
//    off_t loading_process_events_interval_bytes;
    /* Fast pointers to often looked up command */
//    struct redisCommand *delCommand, *multiCommand, *lpushCommand,
//            *lpopCommand, *rpopCommand, *zpopminCommand,
//            *zpopmaxCommand, *sremCommand, *execCommand,
//            *expireCommand, *pexpireCommand, *xclaimCommand,
//            *xgroupCommand, *rpoplpushCommand;
    /* Fields used only for stats */
//    time_t stat_starttime;          /* Server start time */
//    long long stat_numcommands;     /* Number of processed commands */
//    long long stat_numconnections;  /* Number of connections received */
//    long long stat_expiredkeys;     /* Number of expired keys */
//    double stat_expired_stale_perc; /* Percentage of keys probably expired */
//    long long stat_expired_time_cap_reached_count; /* Early expire cylce stops.*/
//    long long stat_expire_cycle_time_used; /* Cumulative microseconds used. */
//    long long stat_evictedkeys;     /* Number of evicted keys (maxmemory) */
//    long long stat_keyspace_hits;   /* Number of successful lookups of keys */
//    long long stat_keyspace_misses; /* Number of failed lookups of keys */
//    long long stat_active_defrag_hits;      /* number of allocations moved */
//    long long stat_active_defrag_misses;    /* number of allocations scanned but not moved */
//    long long stat_active_defrag_key_hits;  /* number of keys with moved allocations */
//    long long stat_active_defrag_key_misses;/* number of keys scanned and not moved */
//    long long stat_active_defrag_scanned;   /* number of dictEntries scanned */
//    size_t stat_peak_memory;        /* Max used memory record */
//    long long stat_fork_time;       /* Time needed to perform latest fork() */
//    double stat_fork_rate;          /* Fork rate in GB/sec. */
    long long stat_rejected_conn;   /* Clients rejected because of maxclients */
//    long long stat_sync_full;       /* Number of full resyncs with slaves. */
//    long long stat_sync_partial_ok; /* Number of accepted PSYNC requests. */
//    long long stat_sync_partial_err;/* Number of unaccepted PSYNC requests. */
//    list *slowlog;                  /* SLOWLOG list of commands */
//    long long slowlog_entry_id;     /* SLOWLOG current entry ID */
//    long long slowlog_log_slower_than; /* SLOWLOG time limit (to get logged) */
//    unsigned long slowlog_max_len;     /* SLOWLOG max number of items logged */
//    struct malloc_stats cron_malloc_stats; /* sampled in serverCron(). */
//    _Atomic long long stat_net_input_bytes; /* Bytes read from network. */
//    _Atomic long long stat_net_output_bytes; /* Bytes written to network. */
//    size_t stat_rdb_cow_bytes;      /* Copy on write bytes during RDB saving. */
//    size_t stat_aof_cow_bytes;      /* Copy on write bytes during AOF rewrite. */
//    size_t stat_module_cow_bytes;   /* Copy on write bytes during module fork. */
//    uint64_t stat_clients_type_memory[CLIENT_TYPE_COUNT];/* Mem usage by type */
//    long long stat_unexpected_error_replies; /* Number of unexpected (aof-loading, replica to master, etc.) error replies */
    /* The following two are used to track instantaneous metrics, like
     * number of operations per second, network traffic. */
//    struct {
//        long long last_sample_time; /* Timestamp of last sample in ms */
//        long long last_sample_count;/* Count in last sample */
//        long long samples[STATS_METRIC_SAMPLES];
//        int idx;
//    } inst_metric[STATS_METRIC_COUNT];
    /* Configuration */
    int verbosity;                  /* Loglevel in redis.conf */
    int maxidletime;                /* Client timeout in seconds */
    int tcpkeepalive;               /* Set SO_KEEPALIVE if non-zero. */
//    int active_expire_enabled;      /* Can be disabled for testing purposes. */
//    int active_expire_effort;       /* From 1 (default) to 10, active effort. */
//    int active_defrag_enabled;
//    int jemalloc_bg_thread;         /* Enable jemalloc background thread */
//    size_t active_defrag_ignore_bytes; /* minimum amount of fragmentation waste to start active defrag */
//    int active_defrag_threshold_lower; /* minimum percentage of fragmentation to start active defrag */
//    int active_defrag_threshold_upper; /* maximum percentage of fragmentation at which we use maximum effort */
//    int active_defrag_cycle_min;       /* minimal effort for defrag in CPU percentage */
//    int active_defrag_cycle_max;       /* maximal effort for defrag in CPU percentage */
//    unsigned long active_defrag_max_scan_fields; /* maximum number of fields of set/hash/zset/list to process from within the main dict scan */
    _Atomic size_t client_max_querybuf_len; /* Limit for client query buffer length */
    int dbnum;                      /* Total number of configured DBs */
//    int supervised;                 /* 1 if supervised, 0 otherwise. */
//    int supervised_mode;            /* See SUPERVISED_* */
    int daemonize;                  /* True if running as a daemon */
//    clientBufferLimitsConfig client_obuf_limits[CLIENT_TYPE_OBUF_COUNT];
    /* AOF persistence */
//    int aof_enabled;                /* AOF configuration */
//    int aof_state;                  /* AOF_(ON|OFF|WAIT_REWRITE) */
//    int aof_fsync;                  /* Kind of fsync() policy */
//    char *aof_filename;             /* Name of the AOF file */
//    int aof_no_fsync_on_rewrite;    /* Don't fsync if a rewrite is in prog. */
//    int aof_rewrite_perc;           /* Rewrite AOF if % growth is > M and... */
//    off_t aof_rewrite_min_size;     /* the AOF file is at least N bytes. */
//    off_t aof_rewrite_base_size;    /* AOF size on latest startup or rewrite. */
//    off_t aof_current_size;         /* AOF current size. */
//    off_t aof_fsync_offset;         /* AOF offset which is already synced to disk. */
//    int aof_flush_sleep;            /* Micros to sleep before flush. (used by tests) */
//    int aof_rewrite_scheduled;      /* Rewrite once BGSAVE terminates. */
//    pid_t aof_child_pid;            /* PID if rewriting process */
//    list *aof_rewrite_buf_blocks;   /* Hold changes during an AOF rewrite. */
//    sds aof_buf;      /* AOF buffer, written before entering the event loop */
//    int aof_fd;       /* File descriptor of currently selected AOF file */
//    int aof_selected_db; /* Currently selected DB in AOF */
//    time_t aof_flush_postponed_start; /* UNIX time of postponed AOF flush */
//    time_t aof_last_fsync;            /* UNIX time of last fsync() */
//    time_t aof_rewrite_time_last;   /* Time used by last AOF rewrite run. */
//    time_t aof_rewrite_time_start;  /* Current AOF rewrite start time. */
//    int aof_lastbgrewrite_status;   /* C_OK or C_ERR */
//    unsigned long aof_delayed_fsync;  /* delayed AOF fsync() counter */
//    int aof_rewrite_incremental_fsync;/* fsync incrementally while aof rewriting? */
//    int rdb_save_incremental_fsync;   /* fsync incrementally while rdb saving? */
//    int aof_last_write_status;      /* C_OK or C_ERR */
//    int aof_last_write_errno;       /* Valid if aof_last_write_status is ERR */
//    int aof_load_truncated;         /* Don't stop on unexpected AOF EOF. */
//    int aof_use_rdb_preamble;       /* Use RDB preamble on AOF rewrites. */
    /* AOF pipes used to communicate between parent and child during rewrite. */
//    int aof_pipe_write_data_to_child;
//    int aof_pipe_read_data_from_parent;
//    int aof_pipe_write_ack_to_parent;
//    int aof_pipe_read_ack_from_child;
//    int aof_pipe_write_ack_to_child;
//    int aof_pipe_read_ack_from_parent;
//    int aof_stop_sending_diff;
    /* If true stop sending accumulated diffs
                                      to child process. */
//    sds aof_child_diff;             /* AOF diff accumulator child side. */
    /* RDB persistence */
//    long long dirty;                /* Changes to DB from the last save */
//    long long dirty_before_bgsave;  /* Used to restore dirty on failed BGSAVE */
//    pid_t rdb_child_pid;            /* PID of RDB saving child */
//    struct saveparam *saveparams;   /* Save points array for RDB */
//    int saveparamslen;              /* Number of saving points */
//    char *rdb_filename;             /* Name of RDB file */
//    int rdb_compression;            /* Use compression in RDB? */
//    int rdb_checksum;               /* Use RDB checksum? */
//    int rdb_del_sync_files;
    /* Remove RDB files used only for SYNC if
                                       the instance does not use persistence. */
//    time_t lastsave;                /* Unix time of last successful save */
//    time_t lastbgsave_try;          /* Unix time of last attempted bgsave */
//    time_t rdb_save_time_last;      /* Time used by last RDB save run. */
//    time_t rdb_save_time_start;     /* Current RDB save start time. */
//    int rdb_bgsave_scheduled;       /* BGSAVE when possible if true. */
//    int rdb_child_type;             /* Type of save by active child. */
//    int lastbgsave_status;          /* C_OK or C_ERR */
//    int stop_writes_on_bgsave_err;  /* Don't allow writes if can't BGSAVE */
//    int rdb_pipe_write;             /* RDB pipes used to transfer the rdb */
//    int rdb_pipe_read;              /* data to the parent process in diskless repl. */
//    connection **rdb_pipe_conns;    /* Connections which are currently the */
//    int rdb_pipe_numconns;          /* target of diskless rdb fork child. */
//    int rdb_pipe_numconns_writing;  /* Number of rdb conns with pending writes. */
//    char *rdb_pipe_buff;            /* In diskless replication, this buffer holds data */
//    int rdb_pipe_bufflen;           /* that was read from the the rdb pipe. */
//    int rdb_key_save_delay;
    /* Delay in microseconds between keys while
                                     * writing the RDB. (for testings) */
//    int key_load_delay;
    /* Delay in microseconds between keys while
                                     * loading aof or rdb. (for testings) */
    /* Pipe and data structures for child -> parent info sharing. */
//    int child_info_pipe[2];         /* Pipe used to write the child_info_data. */
//    struct {
//        int process_type;           /* AOF or RDB child? */
//        size_t cow_size;            /* Copy on write size. */
//        unsigned long long magic;   /* Magic value to make sure data is valid. */
//    } child_info_data;
    /* Propagation of commands in AOF / replication */
//    redisOpArray also_propagate;    /* Additional command to propagate. */
    /* Logging */
    char *logfile;                  /* Path of log file */
    int syslog_enabled;             /* Is syslog enabled? */
    char *syslog_ident;             /* Syslog ident */
    int syslog_facility;            /* Syslog facility */
    /* Replication (master) */
//    char replid[CONFIG_RUN_ID_SIZE+1];  /* My current replication ID. */
//    char replid2[CONFIG_RUN_ID_SIZE+1]; /* replid inherited from master*/
//    long long master_repl_offset;   /* My current replication offset */
//    long long master_repl_meaningful_offset; /* Offset minus latest PINGs. */
//    long long second_replid_offset; /* Accept offsets up to this for replid2. */
//    int slaveseldb;                 /* Last SELECTed DB in replication output */
//    int repl_ping_slave_period;     /* Master pings the slave every N seconds */
//    char *repl_backlog;             /* Replication backlog for partial syncs */
//    long long repl_backlog_size;    /* Backlog circular buffer size */
//    long long repl_backlog_histlen; /* Backlog actual data length */
//    long long repl_backlog_idx;
    /* Backlog circular buffer current offset,
                                       that is the next byte will'll write to.*/
//    long long repl_backlog_off;
    /* Replication "master offset" of first
                                       byte in the replication backlog buffer.*/
//    time_t repl_backlog_time_limit;
    /* Time without slaves after the backlog
                                       gets released. */
//    time_t repl_no_slaves_since;
    /* We have no slaves since that time.
                                       Only valid if server.slaves len is 0. */
//    int repl_min_slaves_to_write;   /* Min number of slaves to write. */
//    int repl_min_slaves_max_lag;    /* Max lag of <count> slaves to write. */
//    int repl_good_slaves_count;     /* Number of slaves with lag <= max_lag. */
//    int repl_diskless_sync;         /* Master send RDB to slaves sockets directly. */
//    int repl_diskless_load;
    /* Slave parse RDB directly from the socket.
                                     * see REPL_DISKLESS_LOAD_* enum */
//    int repl_diskless_sync_delay;   /* Delay to start a diskless repl BGSAVE. */
    /* Replication (slave) */
//    char *masteruser;               /* AUTH with this user and masterauth with master */
//    char *masterauth;               /* AUTH with this password with master */
    char *masterhost;               /* Hostname of master */
    int masterport;                 /* Port of master */
//    int repl_timeout;               /* Timeout after N seconds of master idle */
//    client *master;     /* Client that is master for this slave */
//    client *cached_master; /* Cached master to be reused for PSYNC. */
//    int repl_syncio_timeout; /* Timeout for synchronous I/O calls */
//    int repl_state;          /* Replication status if the instance is a slave */
//    off_t repl_transfer_size; /* Size of RDB to read from master during sync. */
//    off_t repl_transfer_read; /* Amount of RDB read from master during sync. */
//    off_t repl_transfer_last_fsync_off; /* Offset when we fsync-ed last time. */
//    connection *repl_transfer_s;     /* Slave -> Master SYNC connection */
//    int repl_transfer_fd;    /* Slave -> Master SYNC temp file descriptor */
//    char *repl_transfer_tmpfile; /* Slave-> master SYNC temp file name */
//    time_t repl_transfer_lastio; /* Unix time of the latest read, for timeout */
//    int repl_serve_stale_data; /* Serve stale data when link is down? */
//    int repl_slave_ro;          /* Slave is read only? */
//    int repl_slave_ignore_maxmemory;    /* If true slaves do not evict. */
//    time_t repl_down_since; /* Unix time at which link with master went down */
//    int repl_disable_tcp_nodelay;   /* Disable TCP_NODELAY after SYNC? */
//    int slave_priority;             /* Reported in INFO and used by Sentinel. */
//    int slave_announce_port;        /* Give the master this listening port. */
//    char *slave_announce_ip;        /* Give the master this ip address. */
    /* The following two fields is where we store master PSYNC replid/offset
     * while the PSYNC is in progress. At the end we'll copy the fields into
     * the server->master client structure. */
//    char master_replid[CONFIG_RUN_ID_SIZE+1];  /* Master PSYNC runid. */
//    long long master_initial_offset;           /* Master PSYNC offset. */
//    int repl_slave_lazy_flush;          /* Lazy FLUSHALL before loading DB? */
    /* Replication script cache. */
//    dict *repl_scriptcache_dict;        /* SHA1 all slaves are aware of. */
//    list *repl_scriptcache_fifo;        /* First in, first out LRU eviction. */
//    unsigned int repl_scriptcache_size; /* Max number of elements. */
    /* Synchronous replication. */
//    list *clients_waiting_acks;         /* Clients waiting in WAIT command. */
//    int get_ack_from_slaves;            /* If true we send REPLCONF GETACK. */
    /* Limits */
    unsigned int maxclients;            /* Max number of simultaneous clients */
    unsigned long long maxmemory;   /* Max number of memory bytes to use */
//    int maxmemory_policy;           /* Policy for key eviction */
//    int maxmemory_samples;          /* Pricision of random sampling */
//    int lfu_log_factor;             /* LFU logarithmic counter factor. */
//    int lfu_decay_time;             /* LFU counter decay factor. */
//    long long proto_max_bulk_len;   /* Protocol bulk length maximum size. */
    /* Blocked clients */
//    unsigned int blocked_clients;   /* # of clients executing a blocking cmd.*/
//    unsigned int blocked_clients_by_type[BLOCKED_NUM];
//    list *unblocked_clients; /* list of clients to unblock before next loop */
//    list *ready_keys;        /* List of readyList structures for BLPOP & co */
    /* Client side caching. */
//    unsigned int tracking_clients;  /* # of clients with tracking enabled.*/
//    size_t tracking_table_max_keys; /* Max number of keys in tracking table. */
    /* Sort parameters - qsort_r() is only available under BSD so we
     * have to take this state global, in order to pass it to sortCompare() */
//    int sort_desc;
//    int sort_alpha;
//    int sort_bypattern;
//    int sort_store;
    /* Zip structure config, see redis.conf for more information  */
//    size_t hash_max_ziplist_entries;
//    size_t hash_max_ziplist_value;
//    size_t set_max_intset_entries;
//    size_t zset_max_ziplist_entries;
//    size_t zset_max_ziplist_value;
//    size_t hll_sparse_max_bytes;
//    size_t stream_node_max_bytes;
//    long long stream_node_max_entries;
    /* List parameters */
//    int list_max_ziplist_size;
//    int list_compress_depth;
    /* time cache */
    _Atomic time_t unixtime;    /* Unix time sampled every cron cycle. */
    time_t timezone;            /* Cached timezone. As set by tzset(). */
    int daylight_active;        /* Currently in daylight saving time. */
    mstime_t mstime;            /* 'unixtime' in milliseconds. */
    ustime_t ustime;            /* 'unixtime' in microseconds. */
    /* Pubsub */
//    dict *pubsub_channels;  /* Map channels to list of subscribed clients */
//    list *pubsub_patterns;  /* A list of pubsub_patterns */
//    dict *pubsub_patterns_dict;  /* A dict of pubsub_patterns */
//    int notify_keyspace_events;
    /* Events to propagate via Pub/Sub. This is an
                                   xor of NOTIFY_... flags. */
    /* Cluster */
    int cluster_enabled;      /* Is cluster enabled? */
//    mstime_t cluster_node_timeout; /* Cluster node timeout. */
//    char *cluster_configfile; /* Cluster auto-generated config file name. */
//    struct clusterState *cluster;  /* State of the cluster */
//    int cluster_migration_barrier; /* Cluster replicas migration barrier. */
//    int cluster_slave_validity_factor; /* Slave max data age for failover. */
//    int cluster_require_full_coverage;
    /* If true, put the cluster down if
                                          there is at least an uncovered slot.*/
//    int cluster_slave_no_failover;
    /* Prevent slave from starting a failover
                                       if the master is in failure state. */
//    char *cluster_announce_ip;  /* IP address to announce on cluster bus. */
//    int cluster_announce_port;     /* base port to announce on cluster bus. */
//    int cluster_announce_bus_port; /* bus port to announce on cluster bus. */
//    int cluster_module_flags;
    /* Set of flags that tLBS modules are able
                                      to set in order to suppress certain
                                      native tLBS Cluster features. Check the
                                      REDISMODULE_CLUSTER_FLAG_*. */
//    int cluster_allow_reads_when_down;
    /* Are reads allowed when the cluster
                                        is down? */
    /* Scripting */
//    lua_State *lua; /* The Lua interpreter. We use just one for all clients */
//    client *lua_client;   /* The "fake client" to query tLBS from Lua */
//    client *lua_caller;   /* The client running EVAL right now, or NULL */
//    char* lua_cur_script; /* SHA1 of the script currently running, or NULL */
//    dict *lua_scripts;         /* A dictionary of SHA1 -> Lua scripts */
//    unsigned long long lua_scripts_mem;  /* Cached scripts' memory + oh */
//    mstime_t lua_time_limit;  /* Script timeout in milliseconds */
//    mstime_t lua_time_start;  /* Start time of script, milliseconds time */
//    int lua_write_dirty;
    /* True if a write command was called during the
                             execution of the current script. */
//    int lua_random_dirty;
    /* True if a random command was called during the
                             execution of the current script. */
//    int lua_replicate_commands; /* True if we are doing single commands repl. */
//    int lua_multi_emitted;/* True if we already proagated MULTI. */
//    int lua_repl;         /* Script replication flags for redis.set_repl(). */
//    int lua_timedout;
    /* True if we reached the time limit for script
                             execution. */
//    int lua_kill;         /* Kill the script if true. */
//    int lua_always_replicate_commands; /* Default replication type. */
//    int lua_oom;          /* OOM detected when script start? */
    /* Lazy free */
//    int lazyfree_lazy_eviction;
//    int lazyfree_lazy_expire;
//    int lazyfree_lazy_server_del;
//    int lazyfree_lazy_user_del;
    /* Latency monitor */
//    long long latency_monitor_threshold;
//    dict *latency_events;
    /* ACLs */
//    char *acl_filename;     /* ACL Users file. NULL if not configured. */
//    unsigned long acllog_max_len; /* Maximum length of the ACL LOG list. */
//    sds requirepass;
    /* Remember the cleartext password set with the
                               old "requirepass" directive for backward
                               compatibility with tLBS <= 5. */
    /* Assert & bug reporting */
    const char *assert_failed;
    const char *assert_file;
    int assert_line;
    int bug_report_start; /* True if bug report header was already logged. */
//    int watchdog_period;  /* Software watchdog period in ms. 0 = off */
    /* System hardware info */
//    size_t system_memory_size;  /* Total memory in system as reported by OS */
    /* TLS Configuration */
//    int tls_cluster;
//    int tls_replication;
//    int tls_auth_clients;
//    redisTLSContextConfig tls_ctx_config;
};






/*-----------------------------------------------------------------------------
 * Data types 数据类型：点、线、多边形
 *----------------------------------------------------------------------------*/

#define OBJ_POINTS 0 /* (multi-)point 点 */
#define OBJ_LINES 1 /* (multi-)linestring 线 */
#define OBJ_POLYGONS 2 /* (multi-)polygon 多边形 */

#define OBJ_FORMAT_DEGREE 0
#define OBJ_FORMAT_RADIAN 1
#define OBJ_FORMAT_CELL_ID 2


/*--------------------------------------------------------
 * 前缀 point/line/polygon表示要操作的索引的类型
 *--------------------------------------------------------*/
// Point commands
void pointSetCommand(client * c);
void pointGetCommand(client * c);
void pointNearbyCommand(client * c);


// Line commands
void lineSetCommand(client * c);
void lineGetCommand(client * c);
void lineNearbyCommand(client * c);


// Polygon commands
void polygonSetCommand(client * c);
void polygonGetCommand(client * c);
void polygonNearbyCommand(client * c);
void polygonWithinCommand(client * c);


//extern struct tLbsServer server;


/* Configurations */
//void loadServerConfig(char *filename, char *options);
///* Configuration */
//void appendServerSaveParams(time_t seconds, int changes);
//void resetServerSaveParams();
//struct rewriteConfigState; /* Forward declaration to export API. */
//void rewriteConfigRewriteLine(struct rewriteConfigState *state, const char *option, sds line, int force);
//int rewriteConfig(char *path);
//void initConfigValues();


void initServerConfig();
void updateCachedTime(int update_daylight_info);
void daemonize();
void version();
int serverCron(struct aeEventLoop *eventLoop, long long id, void *clientData);
void tLbsSetProcTitle(char *title);
int listenToPort(int port, int *fds, int *count);
void initServer();
void createPidFile();
void beforeSleep(struct aeEventLoop *eventLoop);
void afterSleep(struct aeEventLoop *eventLoop);

void adjustOpenFilesLimit();

extern struct tLbsServer server;

#endif //TLBS_SERVER_H
