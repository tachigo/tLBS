//
// Created by liuliwu on 2020-05-12.
//

#include "debug.h"
#include "common.h"
#include "server.h"
#include <cstdarg>
#include <cstdio>

void _serverPanic(const char *file, int line, const char *msg, ...) {
    va_list ap;
    va_start(ap,msg);
    char fmtmsg[256];
    vsnprintf(fmtmsg,sizeof(fmtmsg),msg,ap);
    va_end(ap);

    bugReportStart();
    serverLog(LL_WARNING,"------------------------------------------------");
    serverLog(LL_WARNING,"!!! Software Failure. Press left mouse button to continue");
    serverLog(LL_WARNING,"Guru Meditation: %s #%s:%d",fmtmsg,file,line);
#ifdef HAVE_BACKTRACE
    serverLog(LL_WARNING,"(forcing SIGSEGV in order to print the stack trace)");
#endif
    serverLog(LL_WARNING,"------------------------------------------------");
    *((char*)-1) = 'x';
}


void bugReportStart() {
    if (server.bug_report_start == 0) {
        serverLogRaw(LL_WARNING|LL_RAW,
                     "\n\n=== tLBS BUG REPORT START: Cut & paste starting from here ===\n");
        server.bug_report_start = 1;
    }
}