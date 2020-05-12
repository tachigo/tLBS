//
// Created by liuliwu on 2020-05-11.
//

#include "common.h"
#include "command.h"
#include "dict.h"
#include "sds.h"
#include "server.h"
#include "debug.h"



/* Command table. sds string -> command struct pointer. */
dictType commandTableDictType = {
        dictSdsCaseHash,            /* hash function */
        nullptr,                       /* key dup */
        nullptr,                       /* val dup */
        dictSdsKeyCaseCompare,      /* key compare */
        dictSdsDestructor,          /* key destructor */
        nullptr                        /* val destructor */
};


struct tLbsCommand tLbsCommandTable[] = {

};


void commandInit() {
    server.commands = dictCreate(&commandTableDictType, nullptr);
    server.orig_commands = dictCreate(&commandTableDictType,nullptr);
    populateCommandTable();
}


void populateCommandTable() {
    int j;
    int numcommands = sizeof(tLbsCommandTable)/sizeof(struct tLbsCommand);

    for (j = 0; j < numcommands; j++) {
        struct tLbsCommand *c = tLbsCommandTable+j;
        int retval1, retval2;

        /* Translate the command string flags description into an actual
         * set of flags. */
        if (populateCommandTableParseFlags(c,c->sflags) == C_ERR)
            serverPanic("Unsupported command flag");

//        c->id = ACLGetCommandID(c->name); /* Assign the ID used for ACL. */
        retval1 = dictAdd(server.commands, sdsnew(c->name), c);
        /* Populate an additional dictionary that will be unaffected
         * by rename-command statements in redis.conf. */
        retval2 = dictAdd(server.orig_commands, sdsnew(c->name), c);
//        serverAssert(retval1 == DICT_OK && retval2 == DICT_OK);
    }
}

int populateCommandTableParseFlags(struct tLbsCommand *c, char *strflags) {
    int argc;
    sds *argv;

    /* Split the line into arguments for processing. */
    argv = sdssplitargs(strflags,&argc);
    if (argv == nullptr) return C_ERR;

//    for (int j = 0; j < argc; j++) {
//        char *flag = argv[j];
//        if (!strcasecmp(flag,"write")) {
//            c->flags |= CMD_WRITE|CMD_CATEGORY_WRITE;
//        } else if (!strcasecmp(flag,"read-only")) {
//            c->flags |= CMD_READONLY|CMD_CATEGORY_READ;
//        } else if (!strcasecmp(flag,"use-memory")) {
//            c->flags |= CMD_DENYOOM;
//        } else if (!strcasecmp(flag,"admin")) {
//            c->flags |= CMD_ADMIN|CMD_CATEGORY_ADMIN|CMD_CATEGORY_DANGEROUS;
//        } else if (!strcasecmp(flag,"pub-sub")) {
//            c->flags |= CMD_PUBSUB|CMD_CATEGORY_PUBSUB;
//        } else if (!strcasecmp(flag,"no-script")) {
//            c->flags |= CMD_NOSCRIPT;
//        } else if (!strcasecmp(flag,"random")) {
//            c->flags |= CMD_RANDOM;
//        } else if (!strcasecmp(flag,"to-sort")) {
//            c->flags |= CMD_SORT_FOR_SCRIPT;
//        } else if (!strcasecmp(flag,"ok-loading")) {
//            c->flags |= CMD_LOADING;
//        } else if (!strcasecmp(flag,"ok-stale")) {
//            c->flags |= CMD_STALE;
//        } else if (!strcasecmp(flag,"no-monitor")) {
//            c->flags |= CMD_SKIP_MONITOR;
//        } else if (!strcasecmp(flag,"no-slowlog")) {
//            c->flags |= CMD_SKIP_SLOWLOG;
//        } else if (!strcasecmp(flag,"cluster-asking")) {
//            c->flags |= CMD_ASKING;
//        } else if (!strcasecmp(flag,"fast")) {
//            c->flags |= CMD_FAST | CMD_CATEGORY_FAST;
//        } else if (!strcasecmp(flag,"no-auth")) {
//            c->flags |= CMD_NO_AUTH;
//        } else {
//            /* Parse ACL categories here if the flag name starts with @. */
//            uint64_t catflag;
//            if (flag[0] == '@' &&
//                (catflag = ACLGetCommandCategoryFlagByName(flag+1)) != 0)
//            {
//                c->flags |= catflag;
//            } else {
//                sdsfreesplitres(argv,argc);
//                return C_ERR;
//            }
//        }
//    }
//    /* If it's not @fast is @slow in this binary world. */
//    if (!(c->flags & CMD_CATEGORY_FAST)) c->flags |= CMD_CATEGORY_SLOW;

    sdsfreesplitres(argv,argc);
    return C_OK;
}


tLbsCommand *lookupCommand(sds name) {
    return (tLbsCommand *)dictFetchValue(server.commands, name);
}