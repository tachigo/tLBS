//
// Created by liuliwu on 2020-05-08.
//

#include "server.h"


#include <sys/time.h>
#include <clocale>
#include <cstdarg>


/*================================= Globals ================================= */

/* Global vars */
struct tLbsServer server; /* Server global state */
double R_Zero, R_PosInf, R_NegInf, R_Nan;
extern char **environ;


/*============================ Utility functions ============================ */

/* We use a private localtime implementation which is fork-safe. The logging
 * function of Redis may be called from other threads. */
void nolocks_localtime(struct tm *tmp, time_t t, time_t tz, int dst);

/* Low level logging. To use only for very big messages, otherwise
 * serverLog() is to prefer. */
void serverLogRaw(int level, const char *msg) {
    const int syslogLevelMap[] = { LOG_DEBUG, LOG_INFO, LOG_NOTICE, LOG_WARNING };
    const char *c = ".-*#";
    FILE *fp;
    char buf[64];
    int rawmode = (level & LL_RAW);
    int log_to_stdout = server.logfile[0] == '\0';

    level &= 0xff; /* clear flags */
    if (level < server.verbosity) return;

    fp = log_to_stdout ? stdout : fopen(server.logfile,"a");
    if (!fp) return;

    if (rawmode) {
        fprintf(fp,"%s",msg);
    } else {
        int off;
        struct timeval tv;
        int role_char;
        pid_t pid = getpid();

        gettimeofday(&tv, nullptr);
        struct tm tm;
        nolocks_localtime(&tm,tv.tv_sec,server.timezone,server.daylight_active);
        off = strftime(buf,sizeof(buf),"%d %b %Y %H:%M:%S.",&tm);
        snprintf(buf+off,sizeof(buf)-off,"%03d",(int)tv.tv_usec/1000);
        if (server.sentinel_mode) {
            role_char = 'X'; /* Sentinel. */
        } else if (pid != server.pid) {
            role_char = 'C'; /* RDB / AOF writing child. */
        } else {
            role_char = (server.masterhost ? 'S':'M'); /* Slave or Master. */
        }
        fprintf(fp,"%d:%c %s %c %s\n",
                (int)getpid(),role_char, buf,c[level],msg);
    }
    fflush(fp);

    if (!log_to_stdout) fclose(fp);
    if (server.syslog_enabled) syslog(syslogLevelMap[level], "%s", msg);
}

/* Like serverLogRaw() but with printf-alike support. This is the function that
 * is used across the code. The raw version is only used in order to dump
 * the INFO output on crash. */
void serverLog(int level, const char *fmt, ...) {
    va_list ap;
    char msg[LOG_MAX_LEN];

    if ((level&0xff) < server.verbosity) return;

    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    serverLogRaw(level,msg);
}

/* Log a fixed message without printf-alike capabilities, in a way that is
 * safe to call from a signal handler.
 *
 * We actually use this only for signals that are not fatal from the point
 * of view of Redis. Signals that are going to kill the server anyway and
 * where we need printf-alike features are served by serverLog(). */
void serverLogFromHandler(int level, const char *msg) {
    int fd;
    int log_to_stdout = server.logfile[0] == '\0';
    char buf[64];

    if ((level & 0xff) < server.verbosity || (log_to_stdout && server.daemonize))
        return;
    fd = log_to_stdout ? STDOUT_FILENO :
         open(server.logfile, O_APPEND | O_CREAT | O_WRONLY, 0644);
    if (fd == -1) return;
    ll2string(buf, sizeof(buf), getpid());
    if (write(fd, buf, strlen(buf)) == -1) goto err;
    if (write(fd, ":signal-handler (", 17) == -1) goto err;
    ll2string(buf, sizeof(buf), time(nullptr));
    if (write(fd, buf, strlen(buf)) == -1) goto err;
    if (write(fd, ") ", 2) == -1) goto err;
    if (write(fd, msg, strlen(msg)) == -1) goto err;
    if (write(fd, "\n", 1) == -1) goto err;
    err:
    if (!log_to_stdout) close(fd);
}

/* Return the UNIX time in microseconds */
long long ustime() {
    struct timeval tv;
    long long ust;

    gettimeofday(&tv, nullptr);
    ust = ((long long)tv.tv_sec)*1000000;
    ust += tv.tv_usec;
    return ust;
}

/* Return the UNIX time in milliseconds */
mstime_t mstime() {
    return ustime()/1000;
}


/* Returns 1 if there is --sentinel among the arguments or if
 * argv[0] contains "redis-sentinel". */
int checkForSentinelMode(int argc, char **argv) {
    int j;

    if (strstr(argv[0],"tlbs-sentinel") != nullptr) return 1;
    for (j = 1; j < argc; j++)
        if (!strcmp(argv[j],"--sentinel")) return 1;
    return 0;
}

/* We take a cached value of the unix time in the global state because with
 * virtual memory and aging there is to store the current time in objects at
 * every object access, and accuracy is not needed. To access a global var is
 * a lot faster than calling time(NULL).
 *
 * This function should be fast because it is called at every command execution
 * in call(), so it is possible to decide if to update the daylight saving
 * info or not using the 'update_daylight_info' argument. Normally we update
 * such info only when calling this function from serverCron() but not when
 * calling it from call(). */
void updateCachedTime(int update_daylight_info) {
    server.ustime = ustime();
    server.mstime = server.ustime / 1000;
    server.unixtime = server.mstime / 1000;

    /* To get information about daylight saving time, we need to call
     * localtime_r and cache the result. However calling localtime_r in this
     * context is safe since we will never fork() while here, in the main
     * thread. The logging function will call a thread safe version of
     * localtime that has no locks. */
    if (update_daylight_info) {
        struct tm tm;
        time_t ut = server.unixtime;
        localtime_r(&ut,&tm);
        server.daylight_active = tm.tm_isdst;
    }
}

void initServerConfig() {
    int j;

    updateCachedTime(1);
    getRandomHexChars(server.runid,CONFIG_RUN_ID_SIZE);
    server.runid[CONFIG_RUN_ID_SIZE] = '\0';
//    changeReplicationId();
//    clearReplicationId2();
//    server.hz = CONFIG_DEFAULT_HZ; /* Initialize it ASAP, even if it may get
//                                      updated later after loading the config.
//                                      This value may be used before the server
//                                      is initialized. */
    server.timezone = getTimeZone(); /* Initialized by tzset(). */
    server.configfile = nullptr;
//    server.executable = nullptr;
//    server.arch_bits = (sizeof(long) == 8) ? 64 : 32;
//    server.bindaddr_count = 0;
//    server.unixsocketperm = CONFIG_DEFAULT_UNIX_SOCKET_PERM;
//    server.ipfd_count = 0;
//    server.tlsfd_count = 0;
//    server.sofd = -1;
//    server.active_expire_enabled = 1;
//    server.client_max_querybuf_len = PROTO_MAX_QUERYBUF_LEN;
//    server.saveparams = NULL;
//    server.loading = 0;
    server.logfile = zstrdup(CONFIG_DEFAULT_LOGFILE);
//    server.aof_state = AOF_OFF;
//    server.aof_rewrite_base_size = 0;
//    server.aof_rewrite_scheduled = 0;
//    server.aof_flush_sleep = 0;
//    server.aof_last_fsync = time(NULL);
//    server.aof_rewrite_time_last = -1;
//    server.aof_rewrite_time_start = -1;
//    server.aof_lastbgrewrite_status = C_OK;
//    server.aof_delayed_fsync = 0;
//    server.aof_fd = -1;
//    server.aof_selected_db = -1; /* Make sure the first time will not match */
//    server.aof_flush_postponed_start = 0;
    server.pidfile = nullptr;
//    server.active_defrag_running = 0;
//    server.notify_keyspace_events = 0;
//    server.blocked_clients = 0;
//    memset(server.blocked_clients_by_type,0,
//           sizeof(server.blocked_clients_by_type));
//    server.shutdown_asap = 0;
//    server.cluster_configfile = zstrdup(CONFIG_DEFAULT_CLUSTER_CONFIG_FILE);
//    server.cluster_module_flags = CLUSTER_MODULE_FLAG_NONE;
//    server.migrate_cached_sockets = dictCreate(&migrateCacheDictType,NULL);
    server.next_client_id = 1; /* Client IDs, start from 1 .*/
//    server.loading_process_events_interval_bytes = (1024*1024*2);

//    server.lruclock = getLRUClock();
//    resetServerSaveParams();

//    appendServerSaveParams(60*60,1);  /* save after 1 hour and 1 change */
//    appendServerSaveParams(300,100);  /* save after 5 minutes and 100 changes */
//    appendServerSaveParams(60,10000); /* save after 1 minute and 10000 changes */

    /* Replication related */
//    server.masterauth = NULL;
//    server.masterhost = NULL;
    server.masterport = 6379;
//    server.master = NULL;
//    server.cached_master = NULL;
//    server.master_initial_offset = -1;
//    server.repl_state = REPL_STATE_NONE;
//    server.repl_transfer_tmpfile = NULL;
//    server.repl_transfer_fd = -1;
//    server.repl_transfer_s = NULL;
//    server.repl_syncio_timeout = CONFIG_REPL_SYNCIO_TIMEOUT;
//    server.repl_down_since = 0; /* Never connected, repl is down since EVER. */
//    server.master_repl_offset = 0;
//    server.master_repl_meaningful_offset = 0;

    /* Replication partial resync backlog */
//    server.repl_backlog = NULL;
//    server.repl_backlog_histlen = 0;
//    server.repl_backlog_idx = 0;
//    server.repl_backlog_off = 0;
//    server.repl_no_slaves_since = time(NULL);

    /* Client output buffer limits */
//    for (j = 0; j < CLIENT_TYPE_OBUF_COUNT; j++)
//        server.client_obuf_limits[j] = clientBufferLimitsDefaults[j];

    /* Double constants initialization */
    R_Zero = 0.0;
    R_PosInf = 1.0/R_Zero;
    R_NegInf = -1.0/R_Zero;
    R_Nan = R_Zero/R_Zero;

    /* Command table -- we initiialize it here as it is part of the
     * initial configuration, since command names may be changed via
     * redis.conf using the rename-command directive. */
//    server.commands = dictCreate(&commandTableDictType,NULL);
//    server.orig_commands = dictCreate(&commandTableDictType,NULL);
//    populateCommandTable();
//    server.delCommand = lookupCommandByCString("del");

    /* Debugging */
//    server.assert_failed = "<no assertion failed>";
//    server.assert_file = "<no file>";
//    server.assert_line = 0;
//    server.bug_report_start = 0;
//    server.watchdog_period = 0;

    /* By default we want scripts to be always replicated by effects
     * (single commands executed by the script), and not by sending the
     * script to the slave / AOF. This is the new way starting from
     * Redis 5. However it is possible to revert it via redis.conf. */
//    server.lua_always_replicate_commands = 1;

    initConfigValues();
}


void daemonize() {
    int fd;

    if (fork() != 0) exit(0); /* parent exits */
    setsid(); /* create a new session */

    /* Every output goes to /dev/null. If Redis is daemonized but
     * the 'logfile' is set to 'stdout' in the configuration file
     * it will not log at all. */
    if ((fd = open("/dev/null", O_RDWR, 0)) != -1) {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        if (fd > STDERR_FILENO) close(fd);
    }
}

void tLbsSetProcTitle(char *title) {
#ifdef USE_SETPROCTITLE
    const char *server_mode = "";
    if (server.cluster_enabled) server_mode = " [cluster]";
    else if (server.sentinel_mode) server_mode = " [sentinel]";

    setproctitle("%s %s:%d%s",
                 title,
                 server.bindaddr_count ? server.bindaddr[0] : "*",
                 server.port ? server.port : server.tls_port,
                 server_mode);
#else
    UNUSED(title);
#endif
}


void initServer() {
    int j;

//    signal(SIGHUP, SIG_IGN);
//    signal(SIGPIPE, SIG_IGN);
//    setupSignalHandlers();

    if (server.syslog_enabled) {
        openlog(server.syslog_ident, LOG_PID | LOG_NDELAY | LOG_NOWAIT,
                server.syslog_facility);
    }

    /* Initialization after setting defaults from the config system. */
//    server.aof_state = server.aof_enabled ? AOF_ON : AOF_OFF;
//    server.hz = server.config_hz;
    server.pid = getpid();
    server.current_client = nullptr;
//    server.fixed_time_expire = 0;
    server.clients = listCreate();
//    server.clients_index = raxNew();
    server.clients_to_close = listCreate();
//    server.slaves = listCreate();
//    server.monitors = listCreate();
    server.clients_pending_write = listCreate();
    server.clients_pending_read = listCreate();
//    server.clients_timeout_table = raxNew();
//    server.slaveseldb = -1; /* Force to emit the first SELECT command. */
    server.unblocked_clients = listCreate();
    server.ready_keys = listCreate();
    server.clients_waiting_acks = listCreate();
//    server.get_ack_from_slaves = 0;
    server.clients_paused = 0;
    server.system_memory_size = zmalloc_get_memory_size();

//    if (server.tls_port && tlsConfigure(&server.tls_ctx_config) == C_ERR) {
//        serverLog(LL_WARNING, "Failed to configure TLS. Check logs for more info.");
//        exit(1);
//    }

//    createSharedObjects();
//    adjustOpenFilesLimit();
    server.el = aeCreateEventLoop(server.maxclients+CONFIG_FDSET_INCR);
    if (server.el == nullptr) {
        serverLog(LL_WARNING,
                  "Failed creating the event loop. Error message: '%s'",
                  strerror(errno));
        exit(1);
    }
    server.db = (tLbsDb *)zmalloc(sizeof(tLbsDb)*server.dbnum);

    /* Open the TCP listening socket for the user commands. */
//    if (server.port != 0 &&
//        listenToPort(server.port,server.ipfd,&server.ipfd_count) == C_ERR)
//        exit(1);
//    if (server.tls_port != 0 &&
//        listenToPort(server.tls_port,server.tlsfd,&server.tlsfd_count) == C_ERR)
//        exit(1);

    /* Open the listening Unix domain socket. */
//    if (server.unixsocket != NULL) {
//        unlink(server.unixsocket); /* don't care if this fails */
//        server.sofd = anetUnixServer(server.neterr,server.unixsocket,
//                                     server.unixsocketperm, server.tcp_backlog);
//        if (server.sofd == ANET_ERR) {
//            serverLog(LL_WARNING, "Opening Unix socket: %s", server.neterr);
//            exit(1);
//        }
//        anetNonBlock(NULL,server.sofd);
//    }

    /* Abort if there are no listening sockets at all. */
//    if (server.ipfd_count == 0 && server.tlsfd_count == 0 && server.sofd < 0) {
//        serverLog(LL_WARNING, "Configured to not listen anywhere, exiting.");
//        exit(1);
//    }

    /* Create the databases, and initialize other internal state. */
//    for (j = 0; j < server.dbnum; j++) {
//        server.db[j].dict = dictCreate(&dbDictType,NULL);
//        server.db[j].expires = dictCreate(&keyptrDictType,NULL);
//        server.db[j].expires_cursor = 0;
//        server.db[j].blocking_keys = dictCreate(&keylistDictType,NULL);
//        server.db[j].ready_keys = dictCreate(&objectKeyPointerValueDictType,NULL);
//        server.db[j].watched_keys = dictCreate(&keylistDictType,NULL);
//        server.db[j].id = j;
//        server.db[j].avg_ttl = 0;
//        server.db[j].defrag_later = listCreate();
//        listSetFreeMethod(server.db[j].defrag_later,(void (*)(void*))sdsfree);
//    }
//    evictionPoolAlloc(); /* Initialize the LRU keys pool. */
//    server.pubsub_channels = dictCreate(&keylistDictType,NULL);
//    server.pubsub_patterns = listCreate();
//    server.pubsub_patterns_dict = dictCreate(&keylistDictType,NULL);
//    listSetFreeMethod(server.pubsub_patterns,freePubsubPattern);
//    listSetMatchMethod(server.pubsub_patterns,listMatchPubsubPattern);
//    server.cronloops = 0;
//    server.rdb_child_pid = -1;
//    server.aof_child_pid = -1;
//    server.module_child_pid = -1;
//    server.rdb_child_type = RDB_CHILD_TYPE_NONE;
//    server.rdb_pipe_conns = NULL;
//    server.rdb_pipe_numconns = 0;
//    server.rdb_pipe_numconns_writing = 0;
//    server.rdb_pipe_buff = NULL;
//    server.rdb_pipe_bufflen = 0;
//    server.rdb_bgsave_scheduled = 0;
//    server.child_info_pipe[0] = -1;
//    server.child_info_pipe[1] = -1;
//    server.child_info_data.magic = 0;
//    aofRewriteBufferReset();
//    server.aof_buf = sdsempty();
//    server.lastsave = time(NULL); /* At startup we consider the DB saved. */
//    server.lastbgsave_try = 0;    /* At startup we never tried to BGSAVE. */
//    server.rdb_save_time_last = -1;
//    server.rdb_save_time_start = -1;
//    server.dirty = 0;
//    resetServerStats();
    /* A few stats we don't want to reset: server startup time, and peak mem. */
//    server.stat_starttime = time(NULL);
//    server.stat_peak_memory = 0;
//    server.stat_rdb_cow_bytes = 0;
//    server.stat_aof_cow_bytes = 0;
//    server.stat_module_cow_bytes = 0;
//    for (int j = 0; j < CLIENT_TYPE_COUNT; j++)
//        server.stat_clients_type_memory[j] = 0;
//    server.cron_malloc_stats.zmalloc_used = 0;
//    server.cron_malloc_stats.process_rss = 0;
//    server.cron_malloc_stats.allocator_allocated = 0;
//    server.cron_malloc_stats.allocator_active = 0;
//    server.cron_malloc_stats.allocator_resident = 0;
//    server.lastbgsave_status = C_OK;
//    server.aof_last_write_status = C_OK;
//    server.aof_last_write_errno = 0;
//    server.repl_good_slaves_count = 0;

    /* Create the timer callback, this is our way to process many background
     * operations incrementally, like clients timeout, eviction of unaccessed
     * expired keys and so forth. */
//    if (aeCreateTimeEvent(server.el, 1, serverCron, NULL, NULL) == AE_ERR) {
//        serverPanic("Can't create event loop timers.");
//        exit(1);
//    }

    /* Create an event handler for accepting new connections in TCP and Unix
     * domain sockets. */
//    for (j = 0; j < server.ipfd_count; j++) {
//        if (aeCreateFileEvent(server.el, server.ipfd[j], AE_READABLE,
//                              acceptTcpHandler,NULL) == AE_ERR)
//        {
//            serverPanic(
//                    "Unrecoverable error creating server.ipfd file event.");
//        }
//    }
//    for (j = 0; j < server.tlsfd_count; j++) {
//        if (aeCreateFileEvent(server.el, server.tlsfd[j], AE_READABLE,
//                              acceptTLSHandler,NULL) == AE_ERR)
//        {
//            serverPanic(
//                    "Unrecoverable error creating server.tlsfd file event.");
//        }
//    }
//    if (server.sofd > 0 && aeCreateFileEvent(server.el,server.sofd,AE_READABLE,
//                                             acceptUnixHandler,NULL) == AE_ERR) serverPanic("Unrecoverable error creating server.sofd file event.");


    /* Register a readable event for the pipe used to awake the event loop
     * when a blocked client in a module needs attention. */
//    if (aeCreateFileEvent(server.el, server.module_blocked_pipe[0], AE_READABLE,
//                          moduleBlockedClientPipeReadable,NULL) == AE_ERR) {
//        serverPanic(
//                "Error registering the readable event for the module "
//                "blocked clients subsystem.");
//    }

    /* Open the AOF file if needed. */
//    if (server.aof_state == AOF_ON) {
//        server.aof_fd = open(server.aof_filename,
//                             O_WRONLY|O_APPEND|O_CREAT,0644);
//        if (server.aof_fd == -1) {
//            serverLog(LL_WARNING, "Can't open the append-only file: %s",
//                      strerror(errno));
//            exit(1);
//        }
//    }

    /* 32 bit instances are limited to 4GB of address space, so if there is
     * no explicit limit in the user provided configuration we set a limit
     * at 3 GB using maxmemory with 'noeviction' policy'. This avoids
     * useless crashes of the Redis instance for out of memory. */
    if (server.arch_bits == 32 && server.maxmemory == 0) {
        serverLog(LL_WARNING,"Warning: 32 bit instance detected but no memory limit set. Setting 3 GB maxmemory limit with 'noeviction' policy now.");
        server.maxmemory = 3072LL*(1024*1024); /* 3 GB */
        server.maxmemory_policy = MAXMEMORY_NO_EVICTION;
    }

//    if (server.cluster_enabled) clusterInit();
//    replicationScriptCacheInit();
//    scriptingInit(1);
//    slowlogInit();
//    latencyMonitorInit();
//    crc64_init();
}

void createPidFile() {
    /* If pidfile requested, but no pidfile defined, use
     * default pidfile path */
    if (!server.pidfile) server.pidfile = zstrdup(CONFIG_DEFAULT_PID_FILE);

    /* Try to write the pid file in a best-effort way. */
    FILE *fp = fopen(server.pidfile,"w");
    if (fp) {
        fprintf(fp,"%d\n",(int)getpid());
        fclose(fp);
    }
}

/* This function gets called every time Redis is entering the
 * main loop of the event driven library, that is, before to sleep
 * for ready file descriptors. */
void beforeSleep(struct aeEventLoop *eventLoop) {
    UNUSED(eventLoop);
}

/* This function is called immadiately after the event loop multiplexing
 * API returned, and the control is going to soon return to Redis by invoking
 * the different events callbacks. */
void afterSleep(struct aeEventLoop *eventLoop) {
    UNUSED(eventLoop);
}

int main(int argc, char **argv) {

    timeval tv;
    int j;

    /* We need to initialize our libraries, and the server configuration. */
#ifdef INIT_SETPROCTITLE_REPLACEMENT
    spt_init(argc, argv);
#endif

    setlocale(LC_COLLATE,"");
    tzset(); /* Populates 'timezone' global. */

    srand(time(nullptr)^getpid());
    gettimeofday(&tv,nullptr);

    uint8_t hashseed[16];
    getRandomBytes(hashseed,sizeof(hashseed));
    dictSetHashFunctionSeed(hashseed);
    server.sentinel_mode = checkForSentinelMode(argc,argv);
    initServerConfig();


    server.executable = getAbsolutePath(argv[0]);
    server.exec_argv = (char **)zmalloc(sizeof(char*)*(argc+1));
    server.exec_argv[argc] = nullptr;
    for (j = 0; j < argc; j++) server.exec_argv[j] = zstrdup(argv[j]);


    serverLog(LL_WARNING, "oO0OoO0OoO0Oo tLBS is starting oO0OoO0OoO0Oo");
    serverLog(LL_WARNING,
              "tLBS version=%s, bits=%d, pid=%d, just started",
              TLBS_VERSION,
              (sizeof(long) == 8) ? 64 : 32,
              (int)getpid());

    if (argc == 1) {
        serverLog(LL_WARNING, "Warning: no config file specified, using the default config. In order to specify a config file use %s /path/to/%s.conf", argv[0], server.sentinel_mode ? "sentinel" : "tLBS");
    } else {
        serverLog(LL_WARNING, "Configuration loaded");
    }

    int background = server.daemonize;
    if (background) daemonize();

    initServer();
    printf("%s", server.pidfile);
    if (background || server.pidfile) createPidFile();

    tLbsSetProcTitle(argv[0]);

    aeSetBeforeSleepProc(server.el,beforeSleep);
    aeSetAfterSleepProc(server.el,afterSleep);
    aeMain(server.el);
    aeDeleteEventLoop(server.el);
    return C_OK;
}