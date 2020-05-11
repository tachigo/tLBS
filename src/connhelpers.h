//
// Created by liuliwu on 2020-05-11.
//

#ifndef TLBS_CONNHELPERS_H
#define TLBS_CONNHELPERS_H

#include "connection.h"

/* These are helper functions that are common to different connection
 * implementations (currently sockets in connection.c and TLS in tls.c).
 *
 * Currently helpers implement the mechanisms for invoking connection
 * handlers and tracking connection references, to allow safe destruction
 * of connections from within a handler.
 */

/* Incremenet connection references.
 *
 * Inside a connection handler, we guarantee refs >= 1 so it is always
 * safe to connClose().
 *
 * In other cases where we don't want to prematurely lose the connection,
 * it can go beyond 1 as well; currently it is only done by connAccept().
 */
static inline void connIncrRefs(connection *conn) {
    conn->refs++;
}

/* Decrement connection references.
 *
 * Note that this is not intended to provide any automatic free logic!
 * callHandler() takes care of that for the common flows, and anywhere an
 * explicit connIncrRefs() is used, the caller is expected to take care of
 * that.
 */

static inline void connDecrRefs(connection *conn) {
    conn->refs--;
}

static inline int connHasRefs(connection *conn) {
    return conn->refs;
}

/* Helper for connection implementations to call handlers:
 * 1. Increment refs to protect the connection.
 * 2. Execute the handler (if set).
 * 3. Decrement refs and perform deferred close, if refs==0.
 */
static inline int callHandler(connection *conn, ConnectionCallbackFunc handler) {
    connIncrRefs(conn);
    if (handler) handler(conn);
    connDecrRefs(conn);
    if (conn->flags & CONN_FLAG_CLOSE_SCHEDULED) {
        if (!connHasRefs(conn)) connClose(conn);
        return 0;
    }
    return 1;
}

#endif //TLBS_CONNHELPERS_H
