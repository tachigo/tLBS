//
// Created by liuliwu on 2020-05-08.
//

#ifndef TLBS_SERVER_H
#define TLBS_SERVER_H

#include "version.h" /* Version macro */

#include "fmacros.h"
#include "config.h"
#include "solarisfixes.h"

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <syslog.h>
#include <fcntl.h>
#include <cstring>
#include <climits>
#include <cerrno>

#include "ae.h"      /* Event driven programming library */
#include "sds.h"     /* Dynamic safe strings */
#include "dict.h"    /* Hash tables */
#include "adlist.h"  /* Linked lists */
#include "zmalloc.h" /* total memory usage aware version of malloc/free */
#include "util.h"


extern double R_Zero, R_PosInf, R_NegInf, R_Nan;

#ifdef __GNUC__
void serverLog(int level, const char *fmt, ...)
__attribute__((format(printf, 2, 3)));
#else
void serverLog(int level, const char *fmt, ...);
#endif
void serverLogRaw(int level, const char *msg);
void serverLogFromHandler(int level, const char *msg);

/* Log levels */
#define LL_DEBUG 0
#define LL_VERBOSE 1
#define LL_NOTICE 2
#define LL_WARNING 3
#define LL_RAW (1<<10) /* Modifier to log without timestamp */
#define LOG_MAX_LEN    1024 /* Default maximum length of syslog messages.*/

/* Anti-warning macro... */
#define UNUSED(V) ((void) V)



typedef long long mstime_t; /* millisecond time type. */
typedef long long ustime_t; /* microsecond time type. */


/* Error codes */
#define C_OK                    0
#define C_ERR                   -1

//struct saveparam {
//    time_t seconds;
//    int changes;
//};

struct tLbsServer;
struct tLbsDb;
struct tLbsClient;
struct tLbsCommand;
struct tLbsObject;








typedef struct tLbsObject {
    unsigned type: 4;
    unsigned format: 4;
    int refcount;
    void * ptr;
} tobj;

typedef struct tLbsDb {
    int id;                     /* Database ID */
    dict *dict;                 /* The keyspace for this DB */
} db;

typedef struct tLbsClient {
    uint64_t id;            /* Client incremental unique ID. */
    tLbsDb *db;
    tobj *obj;

    int argc;               /* Num of arguments of current command. */
    tobj **argv;            /* Arguments of current command. */
    time_t ctime;           /* Client creation time. */
} client;


#define CONFIG_MAX_LINE    1024
#define CONFIG_RUN_ID_SIZE 40
#define CONFIG_DEFAULT_LOGFILE ""
#define CONFIG_BINDADDR_MAX 16
#define CONFIG_DEFAULT_PID_FILE "/var/run/tlbs.pid"

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


#define MAXMEMORY_NO_EVICTION (7<<8)

struct tLbsServer {
    pid_t pid;
    const char * pidfile;
    char *executable;           /* Absolute executable file path. */
    char **exec_argv;           /* Executable argv vector (copy). */
    const char * configfile;
    tLbsDb * db;
    int hz;                     /* serverCron() calls frequency in hertz */
    dict * commands; // 命令表
    int port;
    int tls_port;               /* TLS listening port */
    int tcp_backlog;            /* TCP listen() backlog */
    char *bindaddr[CONFIG_BINDADDR_MAX]; /* Addresses we should bind to */
    int bindaddr_count;         /* Number of addresses in server.bindaddr[] */

    char *masterhost;               /* Hostname of master */
    int masterport;                 /* Port of master */

    int verbosity;                  /* Loglevel in tlbs.conf */
    int sentinel_mode;          /* True if this instance is a Sentinel. */
    char runid[CONFIG_RUN_ID_SIZE+1];  /* ID always different at every exec. */
    int dbnum;                      /* Total number of configured DBs */

    _Atomic uint64_t next_client_id; /* Next client unique ID. Incremental. */

    char *logfile;                  /* Path of log file */
    int syslog_enabled;             /* Is syslog enabled? */
    char *syslog_ident;             /* Syslog ident */
    int syslog_facility;            /* Syslog facility */

    /* Cluster */
    int cluster_enabled;      /* Is cluster enabled? */

    int daemonize;                  /* True if running as a daemon */
    time_t timezone;            /* Cached timezone. As set by tzset(). */
    _Atomic time_t unixtime;    /* Unix time sampled every cron cycle. */
    int daylight_active;        /* Currently in daylight saving time. */
    mstime_t mstime;            /* 'unixtime' in milliseconds. */
    ustime_t ustime;            /* 'unixtime' in microseconds. */

    int loading;                /* We are loading data from disk if true */

    aeEventLoop *el;
    client *current_client;     /* Current client executing the command. */
    list *clients;              /* List of active clients */
    list *clients_to_close;     /* Clients to close asynchronously */
    list *clients_pending_write; /* There is to write or install handler. */
    list *clients_pending_read;  /* Client has pending read socket buffers. */
    /* Blocked clients */
    unsigned int blocked_clients;   /* # of clients executing a blocking cmd.*/
    unsigned int blocked_clients_by_type[BLOCKED_NUM];
    list *unblocked_clients; /* list of clients to unblock before next loop */
    list *ready_keys;        /* List of readyList structures for BLPOP & co */
    list *clients_waiting_acks;         /* Clients waiting in WAIT command. */
    int clients_paused;         /* True if clients are currently paused */


    /* System hardware info */
    size_t system_memory_size;  /* Total memory in system as reported by OS */

    /* Limits */
    unsigned int maxclients;            /* Max number of simultaneous clients */
    unsigned long long maxmemory;   /* Max number of memory bytes to use */
    int maxmemory_policy;           /* Policy for key eviction */
    int maxmemory_samples;          /* Pricision of random sampling */

    int arch_bits;              /* 32 or 64 depending on sizeof(long) */
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


const char * getObjectTypeName(tobj * o);

//const char * getObjectTypeName(tobj * o) {
//    const char * type;
//    if (o == nullptr) {
//        type = "none";
//    }
//    else {
//        switch (o->type) {
//            case OBJ_POINTS: type = "points"; break;
//            case OBJ_LINES: type = "lines"; break;
//            case OBJ_POLYGONS: type = "polygons"; break;
//            default: type = "unknown"; break;
//        }
//    }
//    return type;
//}
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


extern struct tLbsServer server;


/* Utils */
long long ustime();
long long mstime();


/* Configurations */
void loadServerConfig(char *filename, char *options);
/* Configuration */
void appendServerSaveParams(time_t seconds, int changes);
void resetServerSaveParams();
struct rewriteConfigState; /* Forward declaration to export API. */
void rewriteConfigRewriteLine(struct rewriteConfigState *state, const char *option, sds line, int force);
int rewriteConfig(char *path);
void initConfigValues();

#endif //TLBS_SERVER_H
