//
// Created by liuliwu on 2020-05-11.
//

#ifndef TLBS_COMMAND_H
#define TLBS_COMMAND_H

#include <cstdint>
#include "sds.h"



/* Command flags. Please check the command table defined in the redis.c file
 * for more information about the meaning of every flag. */
#define CMD_WRITE (1ULL<<0)            /* "write" flag */
#define CMD_READONLY (1ULL<<1)         /* "read-only" flag */
#define CMD_DENYOOM (1ULL<<2)          /* "use-memory" flag */
#define CMD_MODULE (1ULL<<3)           /* Command exported by module. */
#define CMD_ADMIN (1ULL<<4)            /* "admin" flag */
#define CMD_PUBSUB (1ULL<<5)           /* "pub-sub" flag */
#define CMD_NOSCRIPT (1ULL<<6)         /* "no-script" flag */
#define CMD_RANDOM (1ULL<<7)           /* "random" flag */
#define CMD_SORT_FOR_SCRIPT (1ULL<<8)  /* "to-sort" flag */
#define CMD_LOADING (1ULL<<9)          /* "ok-loading" flag */
#define CMD_STALE (1ULL<<10)           /* "ok-stale" flag */
#define CMD_SKIP_MONITOR (1ULL<<11)    /* "no-monitor" flag */
#define CMD_SKIP_SLOWLOG (1ULL<<12)    /* "no-slowlog" flag */
#define CMD_ASKING (1ULL<<13)          /* "cluster-asking" flag */
#define CMD_FAST (1ULL<<14)            /* "fast" flag */
#define CMD_NO_AUTH (1ULL<<15)         /* "no-auth" flag */

/* Command flags used by the module system. */
#define CMD_MODULE_GETKEYS (1ULL<<16)  /* Use the modules getkeys interface. */
#define CMD_MODULE_NO_CLUSTER (1ULL<<17) /* Deny on Redis Cluster. */


/* Command flags that describe ACLs categories. */
#define CMD_CATEGORY_KEYSPACE (1ULL<<18)
#define CMD_CATEGORY_READ (1ULL<<19)
#define CMD_CATEGORY_WRITE (1ULL<<20)
#define CMD_CATEGORY_SET (1ULL<<21)
#define CMD_CATEGORY_SORTEDSET (1ULL<<22)
#define CMD_CATEGORY_LIST (1ULL<<23)
#define CMD_CATEGORY_HASH (1ULL<<24)
#define CMD_CATEGORY_STRING (1ULL<<25)
#define CMD_CATEGORY_BITMAP (1ULL<<26)
#define CMD_CATEGORY_HYPERLOGLOG (1ULL<<27)
#define CMD_CATEGORY_GEO (1ULL<<28)
#define CMD_CATEGORY_STREAM (1ULL<<29)
#define CMD_CATEGORY_PUBSUB (1ULL<<30)
#define CMD_CATEGORY_ADMIN (1ULL<<31)
#define CMD_CATEGORY_FAST (1ULL<<32)
#define CMD_CATEGORY_SLOW (1ULL<<33)
#define CMD_CATEGORY_BLOCKING (1ULL<<34)
#define CMD_CATEGORY_DANGEROUS (1ULL<<35)
#define CMD_CATEGORY_CONNECTION (1ULL<<36)
#define CMD_CATEGORY_TRANSACTION (1ULL<<37)
#define CMD_CATEGORY_SCRIPTING (1ULL<<38)

struct tLbsCommand {
    char *name;
//    redisCommandProc *proc;
    int arity;
    char *sflags;   /* Flags as string representation, one char per flag. */
    uint64_t flags; /* The actual flags, obtained from the 'sflags' field. */
    /* Use a function to determine keys arguments in a command line.
     * Used for Redis Cluster redirect. */
//    redisGetKeysProc *getkeys_proc;
    /* What keys should be loaded in background when calling this command? */
    int firstkey; /* The first argument that's a key (0 = no keys) */
    int lastkey;  /* The last argument that's a key */
    int keystep;  /* The step between first and last key */
    long long microseconds, calls;
    int id;     /* Command ID. This is a progressive ID starting from 0 that
                   is assigned at runtime, and is used in order to check
                   ACLs. A connection is able to execute a given command if
                   the user associated to the connection has this command
                   bit set in the bitmap of allowed commands. */
};


void commandInit();
int populateCommandTableParseFlags(struct tLbsCommand *c, char *strflags);
void populateCommandTable();
tLbsCommand *lookupCommand(sds name);
//tLbsCommand *lookupCommandByCString(char *s);
//tLbsCommand *lookupCommandOrOriginal(sds name);

#endif //TLBS_COMMAND_H
