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
double R_Zero, R_PosInf, R_NegInf, R_Nan;
extern char **environ;



/* Returns 1 if there is --sentinel among the arguments or if
 * argv[0] contains "redis-sentinel". */
//int checkForSentinelMode(int argc, char **argv) {
//    int j;
//
//    if (strstr(argv[0],"tlbs-sentinel") != nullptr) return 1;
//    for (j = 1; j < argc; j++)
//        if (!strcmp(argv[j],"--sentinel")) return 1;
//    return 0;
//}

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
    server.executable = nullptr;
//    server.arch_bits = (sizeof(long) == 8) ? 64 : 32;
//    server.bindaddr_count = 0;
//    server.unixsocketperm = CONFIG_DEFAULT_UNIX_SOCKET_PERM;
//    server.ipfd_count = 0;
//    server.tlsfd_count = 0;
//    server.sofd = -1;
//    server.active_expire_enabled = 1;
    server.client_max_querybuf_len = PROTO_MAX_QUERYBUF_LEN;
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
//        serverLog(LL_WARNING, "确认shutdown");
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
    if (aeCreateTimeEvent(server.el, 1, serverCron, nullptr, nullptr) == AE_ERR) {
        serverLog(LL_WARNING, "Can't create event loop timers.");
        exit(1);
    }

    /* Create an event handler for accepting new connections in TCP and Unix
     * domain sockets. */
    serverLog(LL_WARNING, "server.ipfd=%d", server.ipfd_count);
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
//    server.sentinel_mode = checkForSentinelMode(argc,argv);
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
    if (background || server.pidfile) createPidFile();

//    serverLog(LL_WARNING, "pid: %d", server.pid);
//    serverLog(LL_WARNING, "pidfile: %s", server.pidfile);
//    serverLog(LL_WARNING, "executable: %s", server.executable);

    tLbsSetProcTitle(argv[0]);

    aeSetBeforeSleepProc(server.el,beforeSleep);
    aeSetAfterSleepProc(server.el,afterSleep);
    aeMain(server.el);
    aeDeleteEventLoop(server.el);
    return C_OK;
}