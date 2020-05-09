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



    return C_OK;
}