//
// Created by liuliwu on 2020-05-09.
//

#ifndef TLBS_CONFIG_H
#define TLBS_CONFIG_H

#ifdef __APPLE__
#include <AvailabilityMacros.h>
#endif

#ifdef __linux__
#include <linux/version.h>
#include <features.h>
#endif

/* Define redis_fstat to fstat or fstat64() */
#if defined(__APPLE__) && !defined(MAC_OS_X_VERSION_10_6)
#define tlbs_fstat fstat64
#define tlbs_stat stat64
#else
#define tlbs_fstat fstat
#define tlbs_stat stat
#endif

/* Test for proc filesystem */
#ifdef __linux__
#define HAVE_PROC_STAT 1
#define HAVE_PROC_MAPS 1
#define HAVE_PROC_SMAPS 1
#define HAVE_PROC_SOMAXCONN 1
#endif

/* Test for task_info() */
#if defined(__APPLE__)
#define HAVE_TASKINFO 1
#endif

/* Test for backtrace() */
#if defined(__APPLE__) || (defined(__linux__) && defined(__GLIBC__)) || \
    defined(__FreeBSD__) || (defined(__OpenBSD__) && defined(USE_BACKTRACE))\
 || defined(__DragonFly__)
#define HAVE_BACKTRACE 1
#endif

/* MSG_NOSIGNAL. */
#ifdef __linux__
#define HAVE_MSG_NOSIGNAL 1
#endif

/* Test for polling API */
#ifdef __linux__
#define HAVE_EPOLL 1
#endif

#if (defined(__APPLE__) && defined(MAC_OS_X_VERSION_10_6)) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined (__NetBSD__)
#define HAVE_KQUEUE 1
#endif


#ifdef __sun
#include <sys/feature_tests.h>
#ifdef _DTRACE_VERSION
#define HAVE_EVPORT 1
#endif
#endif

/* Define redis_fsync to fdatasync() in Linux and fsync() for all the rest */
#ifdef __linux__
#define tlbs_fsync fdatasync
#else
#define tlbs_fsync fsync
#endif


///* Define rdb_fsync_range to sync_file_range() on Linux, otherwise we use
// * the plain fsync() call. */
//#ifdef __linux__
//#if defined(__GLIBC__) && defined(__GLIBC_PREREQ)
//#if (LINUX_VERSION_CODE >= 0x020611 && __GLIBC_PREREQ(2, 6))
//#define HAVE_SYNC_FILE_RANGE 1
//#endif
//#else
//#if (LINUX_VERSION_CODE >= 0x020611)
//#define HAVE_SYNC_FILE_RANGE 1
//#endif
//#endif
//#endif

//#ifdef HAVE_SYNC_FILE_RANGE
//#define rdb_fsync_range(fd,off,size) sync_file_range(fd,off,size,SYNC_FILE_RANGE_WAIT_BEFORE|SYNC_FILE_RANGE_WRITE)
//#else
//#define rdb_fsync_range(fd,off,size) fsync(fd)
//#endif

/* Check if we can use setproctitle().
 * BSD systems have support for it, we provide an implementation for
 * Linux and osx. */
#if (defined __NetBSD__ || defined __FreeBSD__ || defined __OpenBSD__)
#define USE_SETPROCTITLE
#endif

#if ((defined __linux && defined(__GLIBC__)) || defined __APPLE__)
#define USE_SETPROCTITLE
#define INIT_SETPROCTITLE_REPLACEMENT
void spt_init(int argc, char *argv[]);
void setproctitle(const char *fmt, ...);
#endif


/* Byte ordering detection */
#include <sys/types.h> /* This will likely define BYTE_ORDER */


#ifndef BYTE_ORDER
#if (BSD >= 199103)
# include <machine/endian.h>
#else
#if defined(linux) || defined(__linux__)
# include <endian.h>
#else
#define	LITTLE_ENDIAN	1234	/* least-significant byte first (vax, pc) */
#define	BIG_ENDIAN	4321	/* most-significant byte first (IBM, net) */
#define	PDP_ENDIAN	3412	/* LSB first in word, MSW first in long (pdp)*/

#if defined(__i386__) || defined(__x86_64__) || defined(__amd64__) || \
   defined(vax) || defined(ns32000) || defined(sun386) || \
   defined(MIPSEL) || defined(_MIPSEL) || defined(BIT_ZERO_ON_RIGHT) || \
   defined(__alpha__) || defined(__alpha)
#define BYTE_ORDER    LITTLE_ENDIAN
#endif

#if defined(sel) || defined(pyr) || defined(mc68000) || defined(sparc) || \
    defined(is68k) || defined(tahoe) || defined(ibm032) || defined(ibm370) || \
    defined(MIPSEB) || defined(_MIPSEB) || defined(_IBMR2) || defined(DGUX) ||\
    defined(apollo) || defined(__convex__) || defined(_CRAY) || \
    defined(__hppa) || defined(__hp9000) || \
    defined(__hp9000s300) || defined(__hp9000s700) || \
    defined (BIT_ZERO_ON_LEFT) || defined(m68k) || defined(__sparc)
#define BYTE_ORDER	BIG_ENDIAN
#endif
#endif /* linux */
#endif /* BSD */
#endif /* BYTE_ORDER */

/* Sometimes after including an OS-specific header that defines the
 * endianess we end with __BYTE_ORDER but not with BYTE_ORDER that is what
 * the Redis code uses. In this case let's define everything without the
 * underscores. */
#ifndef BYTE_ORDER
#ifdef __BYTE_ORDER
#if defined(__LITTLE_ENDIAN) && defined(__BIG_ENDIAN)
#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN __LITTLE_ENDIAN
#endif
#ifndef BIG_ENDIAN
#define BIG_ENDIAN __BIG_ENDIAN
#endif
#if (__BYTE_ORDER == __LITTLE_ENDIAN)
#define BYTE_ORDER LITTLE_ENDIAN
#else
#define BYTE_ORDER BIG_ENDIAN
#endif
#endif
#endif
#endif

#if !defined(BYTE_ORDER) || \
    (BYTE_ORDER != BIG_ENDIAN && BYTE_ORDER != LITTLE_ENDIAN)
    /* you must determine what the correct bit order is for
	 * your compiler - the next line is an intentional error
	 * which will force your compiles to bomb until you fix
	 * the above macros.
	 */
#error "Undefined or invalid BYTE_ORDER"
#endif

#if (__i386 || __amd64 || __powerpc__) && __GNUC__
#define GNUC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#if defined(__clang__)
#define HAVE_ATOMIC
#endif
#if (defined(__GLIBC__) && defined(__GLIBC_PREREQ))
#if (GNUC_VERSION >= 40100 && __GLIBC_PREREQ(2, 6))
#define HAVE_ATOMIC
#endif
#endif
#endif

/* Make sure we can test for ARM just checking for __arm__, since sometimes
 * __arm is defined but __arm__ is not. */
#if defined(__arm) && !defined(__arm__)
#define __arm__
#endif
#if defined (__aarch64__) && !defined(__arm64__)
#define __arm64__
#endif

/* Make sure we can test for SPARC just checking for __sparc__. */
#if defined(__sparc) && !defined(__sparc__)
#define __sparc__
#endif

#if defined(__sparc__) || defined(__arm__)
#define USE_ALIGNED_ACCESS
#endif

/* Define for tlbs_set_thread_title */
#ifdef __linux__
#define tlbs_set_thread_title(name) pthread_setname_np(pthread_self(), name)
#else
#if (defined __NetBSD__ || defined __FreeBSD__ || defined __OpenBSD__)
#include <pthread_np.h>
#define tlbs_set_thread_title(name) pthread_set_name_np(pthread_self(), name)
#else
#if (defined __APPLE__ && defined(MAC_OS_X_VERSION_10_7))
int pthread_setname_np(char *name);
#include <pthread.h>
#define tlbs_set_thread_title(name) pthread_setname_np(name)
#else
#define tlbs_set_thread_title(name)
#endif
#endif
#endif


#include "common.h"
#include "sds.h"
#include "client.h"

#define DEFAULT_PORT 8899

typedef struct configEnum {
    const char *name;
    const int val;
} configEnum;


/* Generic config infrastructure function pointers
 * int is_valid_fn(val, err)
 *     Return 1 when val is valid, and 0 when invalid.
 *     Optionally set err to a static error string.
 * int update_fn(val, prev, err)
 *     This function is called only for CONFIG SET command (not at config file parsing)
 *     It is called after the actual config is applied,
 *     Return 1 for success, and 0 for failure.
 *     Optionally set err to a static error string.
 *     On failure the config change will be reverted.
 */

/* Configuration values that require no special handling to set, get, load or
 * rewrite. */
typedef struct boolConfigData {
    int *config; /* The pointer to the server config this value is stored in */
    const int default_value; /* The default value of the config on rewrite */
    int (*is_valid_fn)(int val, char **err); /* Optional function to check validity of new value (generic doc above) */
    int (*update_fn)(int val, int prev, char **err); /* Optional function to apply new value at runtime (generic doc above) */
} boolConfigData;

typedef struct stringConfigData {
    char **config; /* Pointer to the server config this value is stored in. */
    const char *default_value; /* Default value of the config on rewrite. */
    int (*is_valid_fn)(char* val, char **err); /* Optional function to check validity of new value (generic doc above) */
    int (*update_fn)(char* val, char* prev, char **err); /* Optional function to apply new value at runtime (generic doc above) */
    int convert_empty_to_null; /* Boolean indicating if empty strings should
                                  be stored as a NULL value. */
} stringConfigData;

typedef struct enumConfigData {
    int *config; /* The pointer to the server config this value is stored in */
    configEnum *enum_value; /* The underlying enum type this data represents */
    const int default_value; /* The default value of the config on rewrite */
    int (*is_valid_fn)(int val, char **err); /* Optional function to check validity of new value (generic doc above) */
    int (*update_fn)(int val, int prev, char **err); /* Optional function to apply new value at runtime (generic doc above) */
} enumConfigData;

typedef enum numericType {
    NUMERIC_TYPE_INT,
    NUMERIC_TYPE_UINT,
    NUMERIC_TYPE_LONG,
    NUMERIC_TYPE_ULONG,
    NUMERIC_TYPE_LONG_LONG,
    NUMERIC_TYPE_ULONG_LONG,
    NUMERIC_TYPE_SIZE_T,
    NUMERIC_TYPE_SSIZE_T,
    NUMERIC_TYPE_OFF_T,
    NUMERIC_TYPE_TIME_T,
} numericType;

typedef struct numericConfigData {
    union {
        int *i;
        unsigned int *ui;
        long *l;
        unsigned long *ul;
        long long *ll;
        unsigned long long *ull;
        size_t *st;
        ssize_t *sst;
        off_t *ot;
        time_t *tt;
    } config; /* The pointer to the numeric config this value is stored in */
    int is_memory; /* Indicates if this value can be loaded as a memory value */
    numericType numeric_type; /* An enum indicating the type of this value */
    long long lower_bound; /* The lower bound of this numeric value */
    long long upper_bound; /* The upper bound of this numeric value */
    const long long default_value; /* The default value of the config on rewrite */
    int (*is_valid_fn)(long long val, char **err); /* Optional function to check validity of new value (generic doc above) */
    int (*update_fn)(long long val, long long prev, char **err); /* Optional function to apply new value at runtime (generic doc above) */
} numericConfigData;

typedef union typeData {
    boolConfigData yesno;
    stringConfigData string;
    enumConfigData enumd;
    numericConfigData numeric;
} typeData;

typedef struct typeInterface {
    /* Called on server start, to init the server with default value */
    void (*init)(typeData data);
    /* Called on server start, should return 1 on success, 0 on error and should set err */
    int (*load)(typeData data, sds *argc, int argv, char **err);
    /* Called on server startup and CONFIG SET, returns 1 on success, 0 on error
     * and can set a verbose err string, update is true when called from CONFIG SET */
    int (*set)(typeData data, sds value, int update, char **err);
    /* Called on CONFIG GET, required to add output to the client */
    void (*get)(client *c, typeData data);
    /* Called on CONFIG REWRITE, required to rewrite the config state */
//    void (*rewrite)(typeData data, const char *name, struct rewriteConfigState *state);
} typeInterface;

typedef struct standardConfig {
    const char *name; /* The user visible name of this config */
    const char *alias; /* An alias that can also be used for this config */
    const int modifiable; /* Can this value be updated by CONFIG SET? */
    typeInterface interface; /* The function pointers that define the type interface */
    typeData data; /* The type specific data exposed used by the interface */
} standardConfig;


/*-----------------------------------------------------------------------------
 * Configs that fit one of the major types and require no special handling
 *----------------------------------------------------------------------------*/
#define LOADBUF_SIZE 256
static char loadbuf[LOADBUF_SIZE];

#define MODIFIABLE_CONFIG 1
#define IMMUTABLE_CONFIG 0

#define ALLOW_EMPTY_STRING 0
#define EMPTY_STRING_IS_NULL 1

#define INTEGER_CONFIG 0
#define MEMORY_CONFIG 1

void loadServerConfigFromString(char *config);
void initConfigValues();
static int updateHZ(long long val, long long prev, char **err);
void loadServerConfig(char *filename, char *options);


int configEnumGetValue(configEnum *ce, char *name);
const char *configEnumGetName(configEnum *ce, int val);
const char *configEnumGetNameOrUnknown(configEnum *ce, int val);

int yesnotoi(char *s);

#define embedCommonConfig(config_name, config_alias, is_modifiable) \
    .name = (config_name), \
    .alias = (config_alias), \
    .modifiable = (is_modifiable),

#define embedConfigInterface(initfn, setfn, getfn) .interface = { \
    .init = (initfn), \
    .set = (setfn), \
    .get = (getfn), \
},

/* Bool Configs */
static void boolConfigInit(typeData data);
static int boolConfigSet(typeData data, sds value, int update, char **err);
static void boolConfigGet(client *c, typeData data);

#define createBoolConfig(name, alias, modifiable, config_addr, default, is_valid, update) { \
    embedCommonConfig(name, alias, modifiable) \
    embedConfigInterface(boolConfigInit, boolConfigSet, boolConfigGet) \
    .data.yesno = { \
        .config = &(config_addr), \
        .default_value = (default), \
        .is_valid_fn = (is_valid), \
        .update_fn = (update), \
    } \
}

/* String Configs */
static void stringConfigInit(typeData data);
static int stringConfigSet(typeData data, sds value, int update, char **err);
static void stringConfigGet(client *c, typeData data);

#define createStringConfig(name, alias, modifiable, empty_to_null, config_addr, default, is_valid, update) { \
    embedCommonConfig(name, alias, modifiable) \
    embedConfigInterface(stringConfigInit, stringConfigSet, stringConfigGet, stringConfigRewrite) \
    .data.string = { \
        .config = &(config_addr), \
        .default_value = (default), \
        .is_valid_fn = (is_valid), \
        .update_fn = (update), \
        .convert_empty_to_null = (empty_to_null), \
    } \
}

/* Enum configs */
static void enumConfigInit(typeData data);
static int enumConfigSet(typeData data, sds value, int update, char **err);
static void enumConfigGet(client *c, typeData data);

#define createEnumConfig(name, alias, modifiable, enum, config_addr, default, is_valid, update) { \
    embedCommonConfig(name, alias, modifiable) \
    embedConfigInterface(enumConfigInit, enumConfigSet, enumConfigGet) \
    .data.enumd = { \
        .config = &(config_addr), \
        .default_value = (default), \
        .is_valid_fn = (is_valid), \
        .update_fn = (update), \
        .enum_value = (enum), \
    } \
}

/* Gets a 'long long val' and sets it into the union, using a macro to get
 * compile time type check. */
#define SET_NUMERIC_TYPE(val) \
    if (data.numeric.numeric_type == NUMERIC_TYPE_INT) { \
        *(data.numeric.config.i) = (int) val; \
    } else if (data.numeric.numeric_type == NUMERIC_TYPE_UINT) { \
        *(data.numeric.config.ui) = (unsigned int) val; \
    } else if (data.numeric.numeric_type == NUMERIC_TYPE_LONG) { \
        *(data.numeric.config.l) = (long) val; \
    } else if (data.numeric.numeric_type == NUMERIC_TYPE_ULONG) { \
        *(data.numeric.config.ul) = (unsigned long) val; \
    } else if (data.numeric.numeric_type == NUMERIC_TYPE_LONG_LONG) { \
        *(data.numeric.config.ll) = (long long) val; \
    } else if (data.numeric.numeric_type == NUMERIC_TYPE_ULONG_LONG) { \
        *(data.numeric.config.ull) = (unsigned long long) val; \
    } else if (data.numeric.numeric_type == NUMERIC_TYPE_SIZE_T) { \
        *(data.numeric.config.st) = (size_t) val; \
    } else if (data.numeric.numeric_type == NUMERIC_TYPE_SSIZE_T) { \
        *(data.numeric.config.sst) = (ssize_t) val; \
    } else if (data.numeric.numeric_type == NUMERIC_TYPE_OFF_T) { \
        *(data.numeric.config.ot) = (off_t) val; \
    } else if (data.numeric.numeric_type == NUMERIC_TYPE_TIME_T) { \
        *(data.numeric.config.tt) = (time_t) val; \
    }

/* Gets a 'long long val' and sets it with the value from the union, using a
 * macro to get compile time type check. */
#define GET_NUMERIC_TYPE(val) \
    if (data.numeric.numeric_type == NUMERIC_TYPE_INT) { \
        val = *(data.numeric.config.i); \
    } else if (data.numeric.numeric_type == NUMERIC_TYPE_UINT) { \
        val = *(data.numeric.config.ui); \
    } else if (data.numeric.numeric_type == NUMERIC_TYPE_LONG) { \
        val = *(data.numeric.config.l); \
    } else if (data.numeric.numeric_type == NUMERIC_TYPE_ULONG) { \
        val = *(data.numeric.config.ul); \
    } else if (data.numeric.numeric_type == NUMERIC_TYPE_LONG_LONG) { \
        val = *(data.numeric.config.ll); \
    } else if (data.numeric.numeric_type == NUMERIC_TYPE_ULONG_LONG) { \
        val = *(data.numeric.config.ull); \
    } else if (data.numeric.numeric_type == NUMERIC_TYPE_SIZE_T) { \
        val = *(data.numeric.config.st); \
    } else if (data.numeric.numeric_type == NUMERIC_TYPE_SSIZE_T) { \
        val = *(data.numeric.config.sst); \
    } else if (data.numeric.numeric_type == NUMERIC_TYPE_OFF_T) { \
        val = *(data.numeric.config.ot); \
    } else if (data.numeric.numeric_type == NUMERIC_TYPE_TIME_T) { \
        val = *(data.numeric.config.tt); \
    }

/* Numeric configs */
static void numericConfigInit(typeData data);
static int numericBoundaryCheck(typeData data, long long ll, char **err);
static int numericConfigSet(typeData data, sds value, int update, char **err);
static void numericConfigGet(client *c, typeData data);

#define embedCommonNumericalConfig(name, alias, modifiable, lower, upper, config_addr, default, memory, is_valid, update) { \
    embedCommonConfig(name, alias, modifiable) \
    embedConfigInterface(numericConfigInit, numericConfigSet, numericConfigGet) \
    .data.numeric = { \
        .lower_bound = (lower), \
        .upper_bound = (upper), \
        .default_value = (default), \
        .is_valid_fn = (is_valid), \
        .update_fn = (update), \
        .is_memory = (memory),

#define createIntConfig(name, alias, modifiable, lower, upper, config_addr, default, memory, is_valid, update) \
    embedCommonNumericalConfig(name, alias, modifiable, lower, upper, config_addr, default, memory, is_valid, update) \
        .numeric_type = NUMERIC_TYPE_INT, \
        .config.i = &(config_addr) \
    } \
}

#define createUIntConfig(name, alias, modifiable, lower, upper, config_addr, default, memory, is_valid, update) \
    embedCommonNumericalConfig(name, alias, modifiable, lower, upper, config_addr, default, memory, is_valid, update) \
        .numeric_type = NUMERIC_TYPE_UINT, \
        .config.ui = &(config_addr) \
    } \
}

#define createLongConfig(name, alias, modifiable, lower, upper, config_addr, default, memory, is_valid, update) \
    embedCommonNumericalConfig(name, alias, modifiable, lower, upper, config_addr, default, memory, is_valid, update) \
        .numeric_type = NUMERIC_TYPE_LONG, \
        .config.l = &(config_addr) \
    } \
}

#define createULongConfig(name, alias, modifiable, lower, upper, config_addr, default, memory, is_valid, update) \
    embedCommonNumericalConfig(name, alias, modifiable, lower, upper, config_addr, default, memory, is_valid, update) \
        .numeric_type = NUMERIC_TYPE_ULONG, \
        .config.ul = &(config_addr) \
    } \
}

#define createLongLongConfig(name, alias, modifiable, lower, upper, config_addr, default, memory, is_valid, update) \
    embedCommonNumericalConfig(name, alias, modifiable, lower, upper, config_addr, default, memory, is_valid, update) \
        .numeric_type = NUMERIC_TYPE_LONG_LONG, \
        .config.ll = &(config_addr) \
    } \
}

#define createULongLongConfig(name, alias, modifiable, lower, upper, config_addr, default, memory, is_valid, update) \
    embedCommonNumericalConfig(name, alias, modifiable, lower, upper, config_addr, default, memory, is_valid, update) \
        .numeric_type = NUMERIC_TYPE_ULONG_LONG, \
        .config.ull = &(config_addr) \
    } \
}

#define createSizeTConfig(name, alias, modifiable, lower, upper, config_addr, default, memory, is_valid, update) \
    embedCommonNumericalConfig(name, alias, modifiable, lower, upper, config_addr, default, memory, is_valid, update) \
        .numeric_type = NUMERIC_TYPE_SIZE_T, \
        .config.st = &(config_addr) \
    } \
}

#define createSSizeTConfig(name, alias, modifiable, lower, upper, config_addr, default, memory, is_valid, update) \
    embedCommonNumericalConfig(name, alias, modifiable, lower, upper, config_addr, default, memory, is_valid, update) \
        .numeric_type = NUMERIC_TYPE_SSIZE_T, \
        .config.sst = &(config_addr) \
    } \
}

#define createTimeTConfig(name, alias, modifiable, lower, upper, config_addr, default, memory, is_valid, update) \
    embedCommonNumericalConfig(name, alias, modifiable, lower, upper, config_addr, default, memory, is_valid, update) \
        .numeric_type = NUMERIC_TYPE_TIME_T, \
        .config.tt = &(config_addr) \
    } \
}

#define createOffTConfig(name, alias, modifiable, lower, upper, config_addr, default, memory, is_valid, update) \
    embedCommonNumericalConfig(name, alias, modifiable, lower, upper, config_addr, default, memory, is_valid, update) \
        .numeric_type = NUMERIC_TYPE_OFF_T, \
        .config.ot = &(config_addr) \
    } \
}



static int updateMaxclients(long long val, long long prev, char **err);

#endif //TLBS_CONFIG_H
