//
// Created by liuliwu on 2020-05-08.
//

#include "server.h"
#include "config.h"
#include "util.h"
#include "zmalloc.h"
#include "version.h"
#include "networking.h"
#include "client.h"
#include "command.h"

#include <sys/time.h>
#include <clocale>
#include <csignal>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <syslog.h>
#include <fcntl.h>
#include <cstring>


/*================================= Globals ================================= */

/* Global vars */
struct tLbsServer server; /* Server global state */
struct sharedObjectsStruct shared;
double R_Zero, R_PosInf, R_NegInf, R_Nan;
extern char **environ;


/* Output buffer limits presets. */
clientBufferLimitsConfig clientBufferLimitsDefaults[CLIENT_TYPE_OBUF_COUNT] = {
        {0, 0, 0}, /* normal */
        {1024*1024*256, 1024*1024*64, 60}, /* slave */
        {1024*1024*32, 1024*1024*8, 60}  /* pubsub */
};

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

void createSharedObjects() {
    int j;

    shared.crlf = createObject(OBJ_TYPE_STRING,sdsnew("\r\n"));
    shared.ok = createObject(OBJ_TYPE_STRING,sdsnew("+OK\r\n"));
    shared.err = createObject(OBJ_TYPE_STRING,sdsnew("-ERR\r\n"));
    shared.emptybulk = createObject(OBJ_TYPE_STRING,sdsnew("$0\r\n\r\n"));
    shared.czero = createObject(OBJ_TYPE_STRING,sdsnew(":0\r\n"));
    shared.cone = createObject(OBJ_TYPE_STRING,sdsnew(":1\r\n"));
    shared.emptyarray = createObject(OBJ_TYPE_STRING,sdsnew("*0\r\n"));
    shared.pong = createObject(OBJ_TYPE_STRING,sdsnew("+PONG\r\n"));
    shared.queued = createObject(OBJ_TYPE_STRING,sdsnew("+QUEUED\r\n"));
    shared.emptyscan = createObject(OBJ_TYPE_STRING,sdsnew("*2\r\n$1\r\n0\r\n*0\r\n"));
    shared.wrongtypeerr = createObject(OBJ_TYPE_STRING,sdsnew(
            "-WRONGTYPE Operation against a key holding the wrong kind of value\r\n"));
    shared.nokeyerr = createObject(OBJ_TYPE_STRING,sdsnew(
            "-ERR no such key\r\n"));
    shared.syntaxerr = createObject(OBJ_TYPE_STRING,sdsnew(
            "-ERR syntax error\r\n"));
    shared.sameobjecterr = createObject(OBJ_TYPE_STRING,sdsnew(
            "-ERR source and destination objects are the same\r\n"));
    shared.outofrangeerr = createObject(OBJ_TYPE_STRING,sdsnew(
            "-ERR index out of range\r\n"));
    shared.noscripterr = createObject(OBJ_TYPE_STRING,sdsnew(
            "-NOSCRIPT No matching script. Please use EVAL.\r\n"));
    shared.loadingerr = createObject(OBJ_TYPE_STRING,sdsnew(
            "-LOADING Redis is loading the dataset in memory\r\n"));
    shared.slowscripterr = createObject(OBJ_TYPE_STRING,sdsnew(
            "-BUSY Redis is busy running a script. You can only call SCRIPT KILL or SHUTDOWN NOSAVE.\r\n"));
    shared.masterdownerr = createObject(OBJ_TYPE_STRING,sdsnew(
            "-MASTERDOWN Link with MASTER is down and replica-serve-stale-data is set to 'no'.\r\n"));
    shared.bgsaveerr = createObject(OBJ_TYPE_STRING,sdsnew(
            "-MISCONF Redis is configured to save RDB snapshots, but it is currently not able to persist on disk. Commands that may modify the data set are disabled, because this instance is configured to report errors during writes if RDB snapshotting fails (stop-writes-on-bgsave-error option). Please check the Redis logs for details about the RDB error.\r\n"));
    shared.roslaveerr = createObject(OBJ_TYPE_STRING,sdsnew(
            "-READONLY You can't write against a read only replica.\r\n"));
    shared.noautherr = createObject(OBJ_TYPE_STRING,sdsnew(
            "-NOAUTH Authentication required.\r\n"));
    shared.oomerr = createObject(OBJ_TYPE_STRING,sdsnew(
            "-OOM command not allowed when used memory > 'maxmemory'.\r\n"));
    shared.execaborterr = createObject(OBJ_TYPE_STRING,sdsnew(
            "-EXECABORT Transaction discarded because of previous errors.\r\n"));
    shared.noreplicaserr = createObject(OBJ_TYPE_STRING,sdsnew(
            "-NOREPLICAS Not enough good replicas to write.\r\n"));
    shared.busykeyerr = createObject(OBJ_TYPE_STRING,sdsnew(
            "-BUSYKEY Target key name already exists.\r\n"));
    shared.space = createObject(OBJ_TYPE_STRING,sdsnew(" "));
    shared.colon = createObject(OBJ_TYPE_STRING,sdsnew(":"));
    shared.plus = createObject(OBJ_TYPE_STRING,sdsnew("+"));

    /* The shared NULL depends on the protocol version. */
    shared.null[0] = nullptr;
    shared.null[1] = nullptr;
    shared.null[2] = createObject(OBJ_TYPE_STRING,sdsnew("$-1\r\n"));
    shared.null[3] = createObject(OBJ_TYPE_STRING,sdsnew("_\r\n"));

    shared.nullarray[0] = nullptr;
    shared.nullarray[1] = nullptr;
    shared.nullarray[2] = createObject(OBJ_TYPE_STRING,sdsnew("*-1\r\n"));
    shared.nullarray[3] = createObject(OBJ_TYPE_STRING,sdsnew("_\r\n"));

    shared.emptymap[0] = nullptr;
    shared.emptymap[1] = nullptr;
    shared.emptymap[2] = createObject(OBJ_TYPE_STRING,sdsnew("*0\r\n"));
    shared.emptymap[3] = createObject(OBJ_TYPE_STRING,sdsnew("%0\r\n"));

    shared.emptyset[0] = nullptr;
    shared.emptyset[1] = nullptr;
    shared.emptyset[2] = createObject(OBJ_TYPE_STRING,sdsnew("*0\r\n"));
    shared.emptyset[3] = createObject(OBJ_TYPE_STRING,sdsnew("~0\r\n"));

    for (j = 0; j < PROTO_SHARED_SELECT_CMDS; j++) {
        char dictid_str[64];
        int dictid_len;

        dictid_len = ll2string(dictid_str,sizeof(dictid_str),j);
        shared.select[j] = createObject(OBJ_TYPE_STRING,
                                        sdscatprintf(sdsempty(),
                                                     "*2\r\n$6\r\nSELECT\r\n$%d\r\n%s\r\n",
                                                     dictid_len, dictid_str));
    }
    shared.messagebulk = createStringObject("$7\r\nmessage\r\n",13);
    shared.pmessagebulk = createStringObject("$8\r\npmessage\r\n",14);
    shared.subscribebulk = createStringObject("$9\r\nsubscribe\r\n",15);
    shared.unsubscribebulk = createStringObject("$11\r\nunsubscribe\r\n",18);
    shared.psubscribebulk = createStringObject("$10\r\npsubscribe\r\n",17);
    shared.punsubscribebulk = createStringObject("$12\r\npunsubscribe\r\n",19);
    shared.del = createStringObject("DEL",3);
    shared.unlink = createStringObject("UNLINK",6);
    shared.rpop = createStringObject("RPOP",4);
    shared.lpop = createStringObject("LPOP",4);
    shared.lpush = createStringObject("LPUSH",5);
    shared.rpoplpush = createStringObject("RPOPLPUSH",9);
    shared.zpopmin = createStringObject("ZPOPMIN",7);
    shared.zpopmax = createStringObject("ZPOPMAX",7);
    shared.multi = createStringObject("MULTI",5);
    shared.exec = createStringObject("EXEC",4);
    for (j = 0; j < OBJ_SHARED_INTEGERS; j++) {
        shared.integers[j] =
                makeObjectShared(createObject(OBJ_TYPE_STRING,(void*)(long)j));
        shared.integers[j]->encoding = OBJ_ENCODING_INT;
    }
    for (j = 0; j < OBJ_SHARED_BULKHDR_LEN; j++) {
        shared.mbulkhdr[j] = createObject(OBJ_TYPE_STRING,
                                          sdscatprintf(sdsempty(),"*%d\r\n",j));
        shared.bulkhdr[j] = createObject(OBJ_TYPE_STRING,
                                         sdscatprintf(sdsempty(),"$%d\r\n",j));
    }
    /* The following two shared objects, minstring and maxstrings, are not
     * actually used for their value but as a special object meaning
     * respectively the minimum possible string and the maximum possible
     * string in string comparisons for the ZRANGEBYLEX command. */
    shared.minstring = sdsnew("minstring");
    shared.maxstring = sdsnew("maxstring");
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
    server.executable = nullptr;
    server.arch_bits = (sizeof(long) == 8) ? 64 : 32;
    server.bindaddr_count = 0;
//    server.unixsocketperm = CONFIG_DEFAULT_UNIX_SOCKET_PERM;
    server.ipfd_count = 0;
//    server.tlsfd_count = 0;
//    server.sofd = -1;
//    server.active_expire_enabled = 1;
    server.client_max_querybuf_len = PROTO_MAX_QUERYBUF_LEN;
//    server.saveparams = NULL;
    server.loading = 0;
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
    server.shutdown_asap = 0;
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
    server.masterhost = nullptr;
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
    for (j = 0; j < CLIENT_TYPE_OBUF_COUNT; j++)
        server.client_obuf_limits[j] = clientBufferLimitsDefaults[j];

    /* Double constants initialization */
    R_Zero = 0.0;
    R_PosInf = 1.0/R_Zero;
    R_NegInf = -1.0/R_Zero;
    R_Nan = R_Zero/R_Zero;

    /* Command table -- we initiialize it here as it is part of the
     * initial configuration, since command names may be changed via
     * redis.conf using the rename-command directive. */
    commandInit();
//    server.commands = dictCreate(&commandTableDictType, nullptr);
//    server.orig_commands = dictCreate(&commandTableDictType,nullptr);
//    populateCommandTable();
//    server.delCommand = lookupCommandByCString("del");

    /* Debugging */
    server.assert_failed = "<no assertion failed>";
    server.assert_file = "<no file>";
    server.assert_line = 0;
    server.bug_report_start = 0;
//    server.watchdog_period = 0;

    /* By default we want scripts to be always replicated by effects
     * (single commands executed by the script), and not by sending the
     * script to the slave / AOF. This is the new way starting from
     * tLBS 5. However it is possible to revert it via redis.conf. */
//    server.lua_always_replicate_commands = 1;

    initConfigValues();
}


void daemonize() {
    int fd;

    if (fork() != 0) exit(0); /* parent exits */
    setsid(); /* create a new session */

    /* Every output goes to /dev/null. If tLBS is daemonized but
     * the 'logfile' is set to 'stdout' in the configuration file
     * it will not log at all. */
    if ((fd = open("/dev/null", O_RDWR, 0)) != -1) {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        if (fd > STDERR_FILENO) close(fd);
    }
}


void version() {
    printf("tLBS server v=%s malloc=%s bits=%d\n",
           TLBS_VERSION,
           ZMALLOC_LIB,
           sizeof(long) == 4 ? 32 : 64);
    exit(0);
}

static void sigShutdownHandler(int sig) {
    const char *msg;

    switch (sig) {
        case SIGINT:
            msg = "Received SIGINT scheduling shutdown...";
            break;
        case SIGTERM:
            msg = "Received SIGTERM scheduling shutdown...";
            break;
        default:
            msg = "Received shutdown signal, scheduling shutdown...";
    };

    /* SIGINT is often delivered via Ctrl+C in an interactive session.
     * If we receive the signal the second time, we interpret this as
     * the user really wanting to quit ASAP without waiting to persist
     * on disk. */
    if (server.shutdown_asap && sig == SIGINT) {
        serverLogFromHandler(LL_WARNING, "You insist... exiting now.");
//        rdbRemoveTempFile(getpid());
        exit(1); /* Exit with an error since this was not a clean shutdown. */
    } else if (server.loading) {
        serverLogFromHandler(LL_WARNING, "Received shutdown signal during loading, exiting now.");
        exit(0);
    }

    serverLogFromHandler(LL_WARNING, msg);
    server.shutdown_asap = 1;
}


void setupSignalHandlers() {
    struct sigaction act;

    /* When the SA_SIGINFO flag is set in sa_flags then sa_sigaction is used.
     * Otherwise, sa_handler is used. */
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    act.sa_handler = sigShutdownHandler;
    sigaction(SIGTERM, &act, nullptr);
    sigaction(SIGINT, &act, nullptr);
    return;
}

int prepareForShutdown(int flags) {
    int save = flags & SHUTDOWN_SAVE;
    int nosave = flags & SHUTDOWN_NOSAVE;

    serverLog(LL_WARNING,"User requested shutdown...");
//    if (server.supervised_mode == SUPERVISED_SYSTEMD)
//        redisCommunicateSystemd("STOPPING=1\n");

    /* Kill all the Lua debugger forked sessions. */
//    ldbKillForkedSessions();

    /* Kill the saving child if there is a background saving in progress.
       We want to avoid race conditions, for instance our saving child may
       overwrite the synchronous saving did by SHUTDOWN. */
//    if (server.rdb_child_pid != -1) {
//        serverLog(LL_WARNING,"There is a child saving an .rdb. Killing it!");
//        killRDBChild();
//    }

    /* Kill module child if there is one. */
//    if (server.module_child_pid != -1) {
//        serverLog(LL_WARNING,"There is a module fork child. Killing it!");
//        TerminateModuleForkChild(server.module_child_pid,0);
//    }

//    if (server.aof_state != AOF_OFF) {
//        /* Kill the AOF saving child as the AOF we already have may be longer
//         * but contains the full dataset anyway. */
//        if (server.aof_child_pid != -1) {
//            /* If we have AOF enabled but haven't written the AOF yet, don't
//             * shutdown or else the dataset will be lost. */
//            if (server.aof_state == AOF_WAIT_REWRITE) {
//                serverLog(LL_WARNING, "Writing initial AOF, can't exit.");
//                return C_ERR;
//            }
//            serverLog(LL_WARNING,
//                      "There is a child rewriting the AOF. Killing it!");
//            killAppendOnlyChild();
//        }
//        /* Append only file: flush buffers and fsync() the AOF at exit */
//        serverLog(LL_NOTICE,"Calling fsync() on the AOF file.");
//        flushAppendOnlyFile(1);
//        redis_fsync(server.aof_fd);
//    }

    /* Create a new RDB file before exiting. */
//    if ((server.saveparamslen > 0 && !nosave) || save) {
//        serverLog(LL_NOTICE,"Saving the final RDB snapshot before exiting.");
//        if (server.supervised_mode == SUPERVISED_SYSTEMD)
//            redisCommunicateSystemd("STATUS=Saving the final RDB snapshot\n");
//        /* Snapshotting. Perform a SYNC SAVE and exit */
//        rdbSaveInfo rsi, *rsiptr;
//        rsiptr = rdbPopulateSaveInfo(&rsi);
//        if (rdbSave(server.rdb_filename,rsiptr) != C_OK) {
//            /* Ooops.. error saving! The best we can do is to continue
//             * operating. Note that if there was a background saving process,
//             * in the next cron() tLBS will be notified that the background
//             * saving aborted, handling special stuff like slaves pending for
//             * synchronization... */
//            serverLog(LL_WARNING,"Error trying to save the DB, can't exit.");
//            if (server.supervised_mode == SUPERVISED_SYSTEMD)
//                redisCommunicateSystemd("STATUS=Error trying to save the DB, can't exit.\n");
//            return C_ERR;
//        }
//    }

    /* Fire the shutdown modules event. */
//    moduleFireServerEvent(REDISMODULE_EVENT_SHUTDOWN,0,NULL);

    /* Remove the pid file if possible and needed. */
    if (server.daemonize || server.pidfile) {
        serverLog(LL_NOTICE,"Removing the pid file.");
        unlink(server.pidfile);
    }

    /* Best effort flush of slave output buffers, so that we hopefully
     * send them pending writes. */
//    flushSlavesOutputBuffers();

    /* Close the listening sockets. Apparently this allows faster restarts. */
    // todo
//    closeListeningSockets(1);
    serverLog(LL_WARNING,"%s is now ready to exit, bye bye...",
              server.sentinel_mode ? "Sentinel" : "tLBS");
    return C_OK;
}

/* This is our timer interrupt, called server.hz times per second.
 * Here is where we do a number of things that need to be done asynchronously.
 * For instance:
 *
 * - Active expired keys collection (it is also performed in a lazy way on
 *   lookup).
 * - Software watchdog.
 * - Update some statistic.
 * - Incremental rehashing of the DBs hash tables.
 * - Triggering BGSAVE / AOF rewrite, and handling of terminated children.
 * - Clients timeout of different kinds.
 * - Replication reconnection.
 * - Many more...
 *
 * Everything directly called here will be called server.hz times per second,
 * so in order to throttle execution of things we want to do less frequently
 * a macro is used: run_with_period(milliseconds) { .... }
 */

int serverCron(struct aeEventLoop *eventLoop, long long id, void *clientData) {
//    serverLog(LL_WARNING, "进入serverCronLoop");
    int j;
    UNUSED(eventLoop);
    UNUSED(id);
    UNUSED(clientData);

    /* Software watchdog: deliver the SIGALRM that will reach the signal
     * handler if we don't return here fast enough. */
//    if (server.watchdog_period) watchdogScheduleSignal(server.watchdog_period);

    /* Update the time cache. */
    updateCachedTime(1);

    server.hz = server.config_hz;
    /* Adapt the server.hz value to the number of configured clients. If we have
     * many clients, we want to call serverCron() with an higher frequency. */
    if (server.dynamic_hz) {
        while (listLength(server.clients) / server.hz >
               MAX_CLIENTS_PER_CLOCK_TICK)
        {
            server.hz *= 2;
            if (server.hz > CONFIG_MAX_HZ) {
                server.hz = CONFIG_MAX_HZ;
                break;
            }
        }
    }
//    serverLog(LL_WARNING, "server.hz=%d", server.hz);

//    run_with_period(100) {
//        trackInstantaneousMetric(STATS_METRIC_COMMAND,server.stat_numcommands);
//        trackInstantaneousMetric(STATS_METRIC_NET_INPUT,
//                                 server.stat_net_input_bytes);
//        trackInstantaneousMetric(STATS_METRIC_NET_OUTPUT,
//                                 server.stat_net_output_bytes);
//    }

    /* We have just LRU_BITS bits per object for LRU information.
     * So we use an (eventually wrapping) LRU clock.
     *
     * Note that even if the counter wraps it's not a big problem,
     * everything will still work but some object will appear younger
     * to Redis. However for this to happen a given object should never be
     * touched for all the time needed to the counter to wrap, which is
     * not likely.
     *
     * Note that you can change the resolution altering the
     * LRU_CLOCK_RESOLUTION define. */
//    server.lruclock = getLRUClock();

    /* Record the max memory used since the server was started. */
//    if (zmalloc_used_memory() > server.stat_peak_memory)
//        server.stat_peak_memory = zmalloc_used_memory();

//    run_with_period(100) {
//        /* Sample the RSS and other metrics here since this is a relatively slow call.
//         * We must sample the zmalloc_used at the same time we take the rss, otherwise
//         * the frag ratio calculate may be off (ratio of two samples at different times) */
//        server.cron_malloc_stats.process_rss = zmalloc_get_rss();
//        server.cron_malloc_stats.zmalloc_used = zmalloc_used_memory();
//        /* Sampling the allcator info can be slow too.
//         * The fragmentation ratio it'll show is potentically more accurate
//         * it excludes other RSS pages such as: shared libraries, LUA and other non-zmalloc
//         * allocations, and allocator reserved pages that can be pursed (all not actual frag) */
//        zmalloc_get_allocator_info(&server.cron_malloc_stats.allocator_allocated,
//                                   &server.cron_malloc_stats.allocator_active,
//                                   &server.cron_malloc_stats.allocator_resident);
//        /* in case the allocator isn't providing these stats, fake them so that
//         * fragmention info still shows some (inaccurate metrics) */
//        if (!server.cron_malloc_stats.allocator_resident) {
//            /* LUA memory isn't part of zmalloc_used, but it is part of the process RSS,
//             * so we must desuct it in order to be able to calculate correct
//             * "allocator fragmentation" ratio */
//            size_t lua_memory = lua_gc(server.lua,LUA_GCCOUNT,0)*1024LL;
//            server.cron_malloc_stats.allocator_resident = server.cron_malloc_stats.process_rss - lua_memory;
//        }
//        if (!server.cron_malloc_stats.allocator_active)
//            server.cron_malloc_stats.allocator_active = server.cron_malloc_stats.allocator_resident;
//        if (!server.cron_malloc_stats.allocator_allocated)
//            server.cron_malloc_stats.allocator_allocated = server.cron_malloc_stats.zmalloc_used;
//    }

    /* We received a SIGTERM, shutting down here in a safe way, as it is
     * not ok doing so inside the signal handler. */
    if (server.shutdown_asap) {
        serverLog(LL_WARNING, "确认shutdown");
        if (prepareForShutdown(SHUTDOWN_NOFLAGS) == C_OK) exit(0);
        serverLog(LL_WARNING,"SIGTERM received but errors trying to shut down the server, check the logs for more information");
        server.shutdown_asap = 0;
    }

    /* Show some info about non-empty databases */
//    run_with_period(5000) {
//        for (j = 0; j < server.dbnum; j++) {
//            long long size, used, vkeys;
//
//            size = dictSlots(server.db[j].dict);
//            used = dictSize(server.db[j].dict);
//            vkeys = dictSize(server.db[j].expires);
//            if (used || vkeys) {
//                serverLog(LL_VERBOSE,"DB %d: %lld keys (%lld volatile) in %lld slots HT.",j,used,vkeys,size);
//                /* dictPrintStats(server.dict); */
//            }
//        }
//    }

    /* Show information about connected clients */
//    if (!server.sentinel_mode) {
//        run_with_period(5000) {
//            serverLog(LL_DEBUG,
//                      "%lu clients connected (%lu replicas), %zu bytes in use",
//                      listLength(server.clients)-listLength(server.slaves),
//                      listLength(server.slaves),
//                      zmalloc_used_memory());
//        }
//    }

    /* We need to do a few operations on clients asynchronously. */
    clientsCron();

    /* Handle background operations on tLBS databases. */
//    databasesCron();

    /* Start a scheduled AOF rewrite if this was requested by the user while
     * a BGSAVE was in progress. */
//    if (!hasActiveChildProcess() &&
//        server.aof_rewrite_scheduled)
//    {
//        rewriteAppendOnlyFileBackground();
//    }

    /* Check if a background saving or AOF rewrite in progress terminated. */
//    if (hasActiveChildProcess() || ldbPendingChildren())
//    {
//        checkChildrenDone();
//    } else {
//        /* If there is not a background saving/rewrite in progress check if
//         * we have to save/rewrite now. */
//        for (j = 0; j < server.saveparamslen; j++) {
//            struct saveparam *sp = server.saveparams+j;
//
//            /* Save if we reached the given amount of changes,
//             * the given amount of seconds, and if the latest bgsave was
//             * successful or if, in case of an error, at least
//             * CONFIG_BGSAVE_RETRY_DELAY seconds already elapsed. */
//            if (server.dirty >= sp->changes &&
//                server.unixtime-server.lastsave > sp->seconds &&
//                (server.unixtime-server.lastbgsave_try >
//                 CONFIG_BGSAVE_RETRY_DELAY ||
//                 server.lastbgsave_status == C_OK))
//            {
//                serverLog(LL_NOTICE,"%d changes in %d seconds. Saving...",
//                          sp->changes, (int)sp->seconds);
//                rdbSaveInfo rsi, *rsiptr;
//                rsiptr = rdbPopulateSaveInfo(&rsi);
//                rdbSaveBackground(server.rdb_filename,rsiptr);
//                break;
//            }
//        }
//
//        /* Trigger an AOF rewrite if needed. */
//        if (server.aof_state == AOF_ON &&
//            !hasActiveChildProcess() &&
//            server.aof_rewrite_perc &&
//            server.aof_current_size > server.aof_rewrite_min_size)
//        {
//            long long base = server.aof_rewrite_base_size ?
//                             server.aof_rewrite_base_size : 1;
//            long long growth = (server.aof_current_size*100/base) - 100;
//            if (growth >= server.aof_rewrite_perc) {
//                serverLog(LL_NOTICE,"Starting automatic rewriting of AOF on %lld%% growth",growth);
//                rewriteAppendOnlyFileBackground();
//            }
//        }
//    }


    /* AOF postponed flush: Try at every cron cycle if the slow fsync
     * completed. */
//    if (server.aof_flush_postponed_start) flushAppendOnlyFile(0);

    /* AOF write errors: in this case we have a buffer to flush as well and
     * clear the AOF error in case of success to make the DB writable again,
     * however to try every second is enough in case of 'hz' is set to
     * an higher frequency. */
//    run_with_period(1000) {
//        if (server.aof_last_write_status == C_ERR)
//            flushAppendOnlyFile(0);
//    }

    /* Clear the paused clients flag if needed. */
//    clientsArePaused(); /* Don't check return value, just use the side effect.*/

    /* Replication cron function -- used to reconnect to master,
     * detect transfer failures, start background RDB transfers and so forth. */
//    run_with_period(1000) replicationCron();

    /* Run the tLBS Cluster cron. */
//    run_with_period(100) {
//        if (server.cluster_enabled) clusterCron();
//    }

    /* Run the Sentinel timer if we are in sentinel mode. */
//    if (server.sentinel_mode) sentinelTimer();

    /* Cleanup expired MIGRATE cached sockets. */
//    run_with_period(1000) {
//        migrateCloseTimedoutSockets();
//    }

    /* Stop the I/O threads if we don't have enough pending work. */
//    stopThreadedIOIfNeeded();

    /* Start a scheduled BGSAVE if the corresponding flag is set. This is
     * useful when we are forced to postpone a BGSAVE because an AOF
     * rewrite is in progress.
     *
     * Note: this code must be after the replicationCron() call above so
     * make sure when refactoring this file to keep this order. This is useful
     * because we want to give priority to RDB savings for replication. */
//    if (!hasActiveChildProcess() &&
//        server.rdb_bgsave_scheduled &&
//        (server.unixtime-server.lastbgsave_try > CONFIG_BGSAVE_RETRY_DELAY ||
//         server.lastbgsave_status == C_OK))
//    {
//        rdbSaveInfo rsi, *rsiptr;
//        rsiptr = rdbPopulateSaveInfo(&rsi);
//        if (rdbSaveBackground(server.rdb_filename,rsiptr) == C_OK)
//            server.rdb_bgsave_scheduled = 0;
//    }

    /* Fire the cron loop modules event. */
//    RedisModuleCronLoopV1 ei = {REDISMODULE_CRON_LOOP_VERSION,server.hz};
//    moduleFireServerEvent(REDISMODULE_EVENT_CRON_LOOP,
//                          0,
//                          &ei);

    server.cronloops++;
    return 1000/server.hz;
}

void tLbsSetProcTitle(char *title) {
#ifdef USE_SETPROCTITLE
    const char *server_mode = "";
    if (server.cluster_enabled) server_mode = " [cluster]";
    else if (server.sentinel_mode) server_mode = " [sentinel]";

    setproctitle("%s %s:%d%s",
                 title,
                 server.bindaddr_count ? server.bindaddr[0] : "*",
                 server.port, // ? server.port : server.tls_port,
                 server_mode);
#else
    UNUSED(title);
#endif
}


int listenToPort(int port, int *fds, int *count) {
    int j;

    /* Force binding of 0.0.0.0 if no bind address is specified, always
     * entering the loop if j == 0. */
    if (server.bindaddr_count == 0) server.bindaddr[0] = nullptr;
    for (j = 0; j < server.bindaddr_count || j == 0; j++) {
        if (server.bindaddr[j] == nullptr) {
            int unsupported = 0;
            /* Bind * for both IPv6 and IPv4, we enter here only if
             * server.bindaddr_count == 0. */
            fds[*count] = anetTcp6Server(server.neterr,port, nullptr,
                                         server.tcp_backlog);
            if (fds[*count] != ANET_ERR) {
                anetNonBlock(nullptr,fds[*count]);
                (*count)++;
            } else if (errno == EAFNOSUPPORT) {
                unsupported++;
                serverLog(LL_WARNING,"Not listening to IPv6: unsupported");
            }

            if (*count == 1 || unsupported) {
                /* Bind the IPv4 address as well. */
                fds[*count] = anetTcpServer(server.neterr,port,nullptr,
                                            server.tcp_backlog);
                if (fds[*count] != ANET_ERR) {
                    anetNonBlock(nullptr,fds[*count]);
                    (*count)++;
                } else if (errno == EAFNOSUPPORT) {
                    unsupported++;
                    serverLog(LL_WARNING,"Not listening to IPv4: unsupported");
                }
            }
            /* Exit the loop if we were able to bind * on IPv4 and IPv6,
             * otherwise fds[*count] will be ANET_ERR and we'll print an
             * error and return to the caller with an error. */
            if (*count + unsupported == 2) break;
        } else if (strchr(server.bindaddr[j],':')) {
            /* Bind IPv6 address. */
            fds[*count] = anetTcp6Server(server.neterr,port,server.bindaddr[j],
                                         server.tcp_backlog);
        } else {
            /* Bind IPv4 address. */
            fds[*count] = anetTcpServer(server.neterr,port,server.bindaddr[j],
                                        server.tcp_backlog);
        }
        if (fds[*count] == ANET_ERR) {
            serverLog(LL_WARNING,
                      "Could not create server TCP listening socket %s:%d: %s",
                      server.bindaddr[j] ? server.bindaddr[j] : "*",
                      port, server.neterr);
            if (errno == ENOPROTOOPT     || errno == EPROTONOSUPPORT ||
                errno == ESOCKTNOSUPPORT || errno == EPFNOSUPPORT ||
                errno == EAFNOSUPPORT    || errno == EADDRNOTAVAIL)
                continue;
            return C_ERR;
        }
        anetNonBlock(nullptr,fds[*count]);
        (*count)++;
    }
    return C_OK;
}


void resetServerStats() {
    int j;

    server.stat_numcommands = 0;
    server.stat_numconnections = 0;
//    server.stat_expiredkeys = 0;
//    server.stat_expired_stale_perc = 0;
//    server.stat_expired_time_cap_reached_count = 0;
//    server.stat_expire_cycle_time_used = 0;
//    server.stat_evictedkeys = 0;
//    server.stat_keyspace_misses = 0;
//    server.stat_keyspace_hits = 0;
//    server.stat_active_defrag_hits = 0;
//    server.stat_active_defrag_misses = 0;
//    server.stat_active_defrag_key_hits = 0;
//    server.stat_active_defrag_key_misses = 0;
//    server.stat_active_defrag_scanned = 0;
//    server.stat_fork_time = 0;
//    server.stat_fork_rate = 0;
    server.stat_rejected_conn = 0;
//    server.stat_sync_full = 0;
//    server.stat_sync_partial_ok = 0;
//    server.stat_sync_partial_err = 0;
//    for (j = 0; j < STATS_METRIC_COUNT; j++) {
//        server.inst_metric[j].idx = 0;
//        server.inst_metric[j].last_sample_time = mstime();
//        server.inst_metric[j].last_sample_count = 0;
//        memset(server.inst_metric[j].samples,0,
//               sizeof(server.inst_metric[j].samples));
//    }
//    server.stat_net_input_bytes = 0;
    server.stat_net_output_bytes = 0;
    server.stat_unexpected_error_replies = 0;
//    server.aof_delayed_fsync = 0;
}

void initServer() {
    int j;

    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    setupSignalHandlers();

    if (server.syslog_enabled) {
        openlog(server.syslog_ident, LOG_PID | LOG_NDELAY | LOG_NOWAIT,
                server.syslog_facility);
    }

    /* Initialization after setting defaults from the config system. */
//    server.aof_state = server.aof_enabled ? AOF_ON : AOF_OFF;
    server.hz = server.config_hz;
    server.pid = getpid();
    server.current_client = nullptr;
//    server.fixed_time_expire = 0;
    server.clients = listCreate();
    server.clients_index = raxNew();
    server.clients_to_close = listCreate();
//    server.slaves = listCreate();
//    server.monitors = listCreate();
    server.clients_pending_write = listCreate();
    server.clients_pending_read = listCreate();
    server.clients_timeout_table = raxNew();
//    server.slaveseldb = -1; /* Force to emit the first SELECT command. */
//    server.unblocked_clients = listCreate();
//    server.ready_keys = listCreate();
//    server.clients_waiting_acks = listCreate();
//    server.get_ack_from_slaves = 0;
    server.clients_paused = 0;
//    server.system_memory_size = zmalloc_get_memory_size();

//    if (server.tls_port && tlsConfigure(&server.tls_ctx_config) == C_ERR) {
//        serverLog(LL_WARNING, "Failed to configure TLS. Check logs for more info.");
//        exit(1);
//    }

    createSharedObjects();
    adjustOpenFilesLimit();
    server.el = aeCreateEventLoop(server.maxclients+CONFIG_FDSET_INCR);
    if (server.el == nullptr) {
        serverLog(LL_WARNING,
                  "Failed creating the event loop. Error message: '%s'",
                  strerror(errno));
        exit(1);
    }
    server.db = (tLbsDb *)zmalloc(sizeof(tLbsDb)*server.dbnum);

    /* Open the TCP listening socket for the user commands. */
    if (server.port != 0 &&
        listenToPort(server.port,server.ipfd,&server.ipfd_count) == C_ERR)
        exit(1);
    serverLog(LL_WARNING, "listen to port: %d", server.port);
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

    dbInit();
//    evictionPoolAlloc(); /* Initialize the LRU keys pool. */
//    server.pubsub_channels = dictCreate(&keylistDictType,NULL);
//    server.pubsub_patterns = listCreate();
//    server.pubsub_patterns_dict = dictCreate(&keylistDictType,NULL);
//    listSetFreeMethod(server.pubsub_patterns,freePubsubPattern);
//    listSetMatchMethod(server.pubsub_patterns,listMatchPubsubPattern);
    server.cronloops = 0;
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
    server.dirty = 0;
    resetServerStats();
    /* A few stats we don't want to reset: server startup time, and peak mem. */
    server.stat_starttime = time(nullptr);
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
    if (aeCreateTimeEvent(server.el, 1, serverCron, nullptr, nullptr) == AE_ERR) {
        serverLog(LL_WARNING, "Can't create event loop timers.");
        exit(1);
    }

    /* Create an event handler for accepting new connections in TCP and Unix
     * domain sockets. */
//    serverLog(LL_WARNING, "server.ipfd=%d", server.ipfd_count);
    for (j = 0; j < server.ipfd_count; j++) {
        if (aeCreateFileEvent(server.el, server.ipfd[j], AE_READABLE,
                              acceptTcpHandler, nullptr) == AE_ERR)
        {
            serverLog(LL_WARNING,
                    "Unrecoverable error creating server.ipfd file event.");
        }
    }
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
     * useless crashes of the tLBS instance for out of memory. */
//    if (server.arch_bits == 32 && server.maxmemory == 0) {
//        serverLog(LL_WARNING,"Warning: 32 bit instance detected but no memory limit set. Setting 3 GB maxmemory limit with 'noeviction' policy now.");
//        server.maxmemory = 3072LL*(1024*1024); /* 3 GB */
//        server.maxmemory_policy = MAXMEMORY_NO_EVICTION;
//    }

//    if (server.cluster_enabled) clusterInit();
//    replicationScriptCacheInit();
//    scriptingInit(1);
//    slowlogInit();
//    latencyMonitorInit();
//    crc64_init();
}

void initServerLast() {
//    bioInit();
    initThreadedIO();
//    set_jemalloc_bg_thread(server.jemalloc_bg_thread);
//    server.initial_memory_usage = zmalloc_used_memory();
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

/* This function gets called every time tLBS is entering the
 * main loop of the event driven library, that is, before to sleep
 * for ready file descriptors. */
void beforeSleep(struct aeEventLoop *eventLoop) {
    UNUSED(eventLoop);

    /* Handle precise timeouts of blocked clients. */
//    handleBlockedClientsTimeout();

    /* We should handle pending reads clients ASAP after event loop. */
    handleClientsWithPendingReadsUsingThreads();

    /* Handle TLS pending data. (must be done before flushAppendOnlyFile) */
//    tlsProcessPendingData();

    /* If tls still has pending unread data don't sleep at all. */
//    aeSetDontWait(server.el, tlsHasPendingData());

    /* Call the Redis Cluster before sleep function. Note that this function
     * may change the state of Redis Cluster (from ok to fail or vice versa),
     * so it's a good idea to call it before serving the unblocked clients
     * later in this function. */
//    if (server.cluster_enabled) clusterBeforeSleep();

    /* Run a fast expire cycle (the called function will return
     * ASAP if a fast cycle is not needed). */
//    if (server.active_expire_enabled && server.masterhost == NULL)
//        activeExpireCycle(ACTIVE_EXPIRE_CYCLE_FAST);

    /* Unblock all the clients blocked for synchronous replication
     * in WAIT. */
//    if (listLength(server.clients_waiting_acks))
//        processClientsWaitingReplicas();

    /* Check if there are clients unblocked by modules that implement
     * blocking commands. */
//    if (moduleCount()) moduleHandleBlockedClients();

    /* Try to process pending commands for clients that were just unblocked. */
//    if (listLength(server.unblocked_clients))
//        processUnblockedClients();

    /* Send all the slaves an ACK request if at least one client blocked
     * during the previous event loop iteration. Note that we do this after
     * processUnblockedClients(), so if there are multiple pipelined WAITs
     * and the just unblocked WAIT gets blocked again, we don't have to wait
     * a server cron cycle in absence of other event loop events. See #6623. */
//    if (server.get_ack_from_slaves) {
//        robj *argv[3];
//
//        argv[0] = createStringObject("REPLCONF",8);
//        argv[1] = createStringObject("GETACK",6);
//        argv[2] = createStringObject("*",1); /* Not used argument. */
//        replicationFeedSlaves(server.slaves, server.slaveseldb, argv, 3);
//        decrRefCount(argv[0]);
//        decrRefCount(argv[1]);
//        decrRefCount(argv[2]);
//        server.get_ack_from_slaves = 0;
//    }

    /* Send the invalidation messages to clients participating to the
     * client side caching protocol in broadcasting (BCAST) mode. */
//    trackingBroadcastInvalidationMessages();

    /* Write the AOF buffer on disk */
//    flushAppendOnlyFile(0);

    /* Handle writes with pending output buffers. */
    handleClientsWithPendingWritesUsingThreads();

    /* Close clients that need to be closed asynchronous */
    freeClientsInAsyncFreeQueue();

    /* Before we are going to sleep, let the threads access the dataset by
     * releasing the GIL. Redis main thread will not touch anything at this
     * time. */
//    if (moduleCount()) moduleReleaseGIL();
}

/* This function is called immadiately after the event loop multiplexing
 * API returned, and the control is going to soon return to tLBS by invoking
 * the different events callbacks. */
void afterSleep(struct aeEventLoop *eventLoop) {
    UNUSED(eventLoop);
}

void adjustOpenFilesLimit() {
    rlim_t maxfiles = server.maxclients+CONFIG_MIN_RESERVED_FDS;
    struct rlimit limit;

    if (getrlimit(RLIMIT_NOFILE,&limit) == -1) {
        serverLog(LL_WARNING,"Unable to obtain the current NOFILE limit (%s), assuming 1024 and setting the max clients configuration accordingly.",
                  strerror(errno));
        server.maxclients = 1024-CONFIG_MIN_RESERVED_FDS;
    } else {
        rlim_t oldlimit = limit.rlim_cur;

        /* Set the max number of files if the current limit is not enough
         * for our needs. */
        if (oldlimit < maxfiles) {
            rlim_t bestlimit;
            int setrlimit_error = 0;

            /* Try to set the file limit to match 'maxfiles' or at least
             * to the higher value supported less than maxfiles. */
            bestlimit = maxfiles;
            while(bestlimit > oldlimit) {
                rlim_t decr_step = 16;

                limit.rlim_cur = bestlimit;
                limit.rlim_max = bestlimit;
                if (setrlimit(RLIMIT_NOFILE,&limit) != -1) break;
                setrlimit_error = errno;

                /* We failed to set file limit to 'bestlimit'. Try with a
                 * smaller limit decrementing by a few FDs per iteration. */
                if (bestlimit < decr_step) break;
                bestlimit -= decr_step;
            }

            /* Assume that the limit we get initially is still valid if
             * our last try was even lower. */
            if (bestlimit < oldlimit) bestlimit = oldlimit;

            if (bestlimit < maxfiles) {
                unsigned int old_maxclients = server.maxclients;
                server.maxclients = bestlimit-CONFIG_MIN_RESERVED_FDS;
                /* maxclients is unsigned so may overflow: in order
                 * to check if maxclients is now logically less than 1
                 * we test indirectly via bestlimit. */
                if (bestlimit <= CONFIG_MIN_RESERVED_FDS) {
                    serverLog(LL_WARNING,"Your current 'ulimit -n' "
                                         "of %llu is not enough for the server to start. "
                                         "Please increase your open file limit to at least "
                                         "%llu. Exiting.",
                              (unsigned long long) oldlimit,
                              (unsigned long long) maxfiles);
                    exit(1);
                }
                serverLog(LL_WARNING,"You requested maxclients of %d "
                                     "requiring at least %llu max file descriptors.",
                          old_maxclients,
                          (unsigned long long) maxfiles);
                serverLog(LL_WARNING,"Server can't set maximum open files "
                                     "to %llu because of OS error: %s.",
                          (unsigned long long) maxfiles, strerror(setrlimit_error));
                serverLog(LL_WARNING,"Current maximum open files is %llu. "
                                     "maxclients has been reduced to %d to compensate for "
                                     "low ulimit. "
                                     "If you need higher maxclients increase 'ulimit -n'.",
                          (unsigned long long) bestlimit, server.maxclients);
            } else {
                serverLog(LL_NOTICE,"Increased maximum number of open files "
                                    "to %llu (it was originally set to %llu).",
                          (unsigned long long) maxfiles,
                          (unsigned long long) oldlimit);
            }
        }
    }
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

    /* We need to init sentinel right now as parsing the configuration file
     * in sentinel mode will have the effect of populating the sentinel
     * data structures with master nodes to monitor. */
//    if (server.sentinel_mode) {
//        initSentinelConfig();
//        initSentinel();
//    }

    /* Check if we need to start in redis-check-rdb/aof mode. We just execute
     * the program main. However the program is part of the Redis executable
     * so that we can easily execute an RDB check on loading errors. */
//    if (strstr(argv[0],"redis-check-rdb") != NULL)
//        redis_check_rdb_main(argc,argv,NULL);
//    else if (strstr(argv[0],"redis-check-aof") != NULL)
//        redis_check_aof_main(argc,argv);

//    if (argc >= 2) {
//        j = 1; /* First option to parse in argv[] */
//        sds options = sdsempty();
//        char *configfile = nullptr;
//
//        /* Handle special options --help and --version */
//        if (strcmp(argv[1], "-v") == 0 ||
//            strcmp(argv[1], "--version") == 0) version();
//        if (strcmp(argv[1], "--help") == 0 ||
//            strcmp(argv[1], "-h") == 0) usage();
//        if (strcmp(argv[1], "--test-memory") == 0) {
//            if (argc == 3) {
//                memtest(atoi(argv[2]),50);
//                exit(0);
//            } else {
//                fprintf(stderr,"Please specify the amount of memory to test in megabytes.\n");
//                fprintf(stderr,"Example: ./redis-server --test-memory 4096\n\n");
//                exit(1);
//            }
//        }
//
//        /* First argument is the config file name? */
//        if (argv[j][0] != '-' || argv[j][1] != '-') {
//            configfile = argv[j];
//            server.configfile = getAbsolutePath(configfile);
//            /* Replace the config file in server.exec_argv with
//             * its absolute path. */
//            zfree(server.exec_argv[j]);
//            server.exec_argv[j] = zstrdup(server.configfile);
//            j++;
//        }
//
//        /* All the other options are parsed and conceptually appended to the
//         * configuration file. For instance --port 6380 will generate the
//         * string "port 6380\n" to be parsed after the actual file name
//         * is parsed, if any. */
//        while(j != argc) {
//            if (argv[j][0] == '-' && argv[j][1] == '-') {
//                /* Option name */
//                if (!strcmp(argv[j], "--check-rdb")) {
//                    /* Argument has no options, need to skip for parsing. */
//                    j++;
//                    continue;
//                }
//                if (sdslen(options)) options = sdscat(options,"\n");
//                options = sdscat(options,argv[j]+2);
//                options = sdscat(options," ");
//            } else {
//                /* Option argument */
//                options = sdscatrepr(options,argv[j],strlen(argv[j]));
//                options = sdscat(options," ");
//            }
//            j++;
//        }
//        if (server.sentinel_mode && configfile && *configfile == '-') {
//            serverLog(LL_WARNING,
//                      "Sentinel config from STDIN not allowed.");
//            serverLog(LL_WARNING,
//                      "Sentinel needs config file on disk to save state.  Exiting...");
//            exit(1);
//        }
//        resetServerSaveParams();
//        loadServerConfig(configfile,options);
//        sdsfree(options);
//    }


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
    if (background || server.pidfile) createPidFile();

    serverLog(LL_WARNING, "aeApiName: `%s`", aeGetApiName());

    tLbsSetProcTitle(argv[0]);

    if (!server.sentinel_mode) {
        /* Things not needed when running in Sentinel mode. */
        serverLog(LL_WARNING,"Server initialized");
#ifdef __linux__
        linuxMemoryWarnings();
#endif
//        moduleLoadFromQueue();
//        ACLLoadUsersAtStartup();
//        InitServerLast();
//        loadDataFromDisk();
//        if (server.cluster_enabled) {
//            if (verifyClusterConfigWithData() == C_ERR) {
//                serverLog(LL_WARNING,
//                          "You can't have keys in a DB different than DB 0 when in "
//                          "Cluster mode. Exiting.");
//                exit(1);
//            }
//        }
        serverLog(LL_NOTICE,"Ready to accept connections, server.ipfd_count=%d", server.ipfd_count);
//        if (server.ipfd_count > 0 || server.tlsfd_count > 0)
//            serverLog(LL_NOTICE,"Ready to accept connections");
//        if (server.sofd > 0)
//            serverLog(LL_NOTICE,"The server is now ready to accept connections at %s", server.unixsocket);
//        if (server.supervised_mode == SUPERVISED_SYSTEMD) {
//            if (!server.masterhost) {
//                redisCommunicateSystemd("STATUS=Ready to accept connections\n");
//                redisCommunicateSystemd("READY=1\n");
//            } else {
//                redisCommunicateSystemd("STATUS=Waiting for MASTER <-> REPLICA sync\n");
//            }
//        }
    } else {
        initServerLast();
//        sentinelIsRunning();
    }

    /* Warning the user about suspicious maxmemory setting. */
    if (server.maxmemory > 0 && server.maxmemory < 1024*1024) {
        serverLog(LL_WARNING,"WARNING: You specified a maxmemory value that is less than 1MB (current value is %llu bytes). Are you sure this is what you really want?", server.maxmemory);
    }

    aeSetBeforeSleepProc(server.el,beforeSleep);
    aeSetAfterSleepProc(server.el,afterSleep);
    aeMain(server.el);
    aeDeleteEventLoop(server.el);
    return C_OK;
}