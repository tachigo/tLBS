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

#define CONFIG_MAX_LINE    1024
#define CONFIG_RUN_ID_SIZE 40
#define CONFIG_DEFAULT_LOGFILE ""
#define CONFIG_BINDADDR_MAX 16

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
    int daemonize;                  /* True if running as a daemon */
    time_t timezone;            /* Cached timezone. As set by tzset(). */
    _Atomic time_t unixtime;    /* Unix time sampled every cron cycle. */
    int daylight_active;        /* Currently in daylight saving time. */
    mstime_t mstime;            /* 'unixtime' in milliseconds. */
    ustime_t ustime;            /* 'unixtime' in microseconds. */

    int loading;                /* We are loading data from disk if true */
};


typedef struct tLbsDb {
    int id;                     /* Database ID */
    dict *dict;                 /* The keyspace for this DB */
} db;

typedef struct tLbsObject {
    unsigned type: 4;
    unsigned format: 4;
    int refcount;
    void * ptr;
} tobj;


typedef struct tLbsClient {
    uint64_t id;            /* Client incremental unique ID. */
    tLbsDb *db;
    tobj *obj;

    int argc;               /* Num of arguments of current command. */
    tobj **argv;            /* Arguments of current command. */
    time_t ctime;           /* Client creation time. */
} client;




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
