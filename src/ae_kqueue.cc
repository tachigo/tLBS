//
// Created by liuliwu on 2020-05-09.
//

//#include "ae.h"
//#include "zmalloc.h"
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

typedef struct aeApiState {
    int kqfd;
    struct kevent *events;
} aeApiState;

static int aeApiCreate(aeEventLoop *eventLoop) {
    auto *state = (aeApiState *)zmalloc(sizeof(aeApiState));

    if (!state) return -1;
    state->events = (struct kevent *)zmalloc(sizeof(struct kevent)*eventLoop->setsize);
    if (!state->events) {
        zfree(state);
        return -1;
    }
    state->kqfd = kqueue();
    if (state->kqfd == -1) {
        zfree(state->events);
        zfree(state);
        return -1;
    }
    eventLoop->apidata = state;
    return 0;
}

static int aeApiResize(aeEventLoop *eventLoop, int setsize) {
    auto *state = (aeApiState *)eventLoop->apidata;

    state->events = (struct kevent *)zrealloc(state->events, sizeof(struct kevent)*setsize);
    return 0;
}

static void aeApiFree(aeEventLoop *eventLoop) {
    auto *state = (aeApiState *)eventLoop->apidata;

    close(state->kqfd);
    zfree(state->events);
    zfree(state);
}

static int aeApiAddEvent(aeEventLoop *eventLoop, int fd, int mask) {
    auto *state = (aeApiState *)eventLoop->apidata;
    struct kevent ke;

    if (mask & AE_READABLE) {
        EV_SET(&ke, fd, EVFILT_READ, EV_ADD, 0, 0, nullptr);
        if (kevent(state->kqfd, &ke, 1, nullptr, 0, nullptr) == -1) return -1;
    }
    if (mask & AE_WRITABLE) {
        EV_SET(&ke, fd, EVFILT_WRITE, EV_ADD, 0, 0, nullptr);
        if (kevent(state->kqfd, &ke, 1, nullptr, 0, nullptr) == -1) return -1;
    }
    return 0;
}

static void aeApiDelEvent(aeEventLoop *eventLoop, int fd, int mask) {
    auto *state = (aeApiState *)eventLoop->apidata;
    struct kevent ke;

    if (mask & AE_READABLE) {
        EV_SET(&ke, fd, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
        kevent(state->kqfd, &ke, 1, nullptr, 0, nullptr);
    }
    if (mask & AE_WRITABLE) {
        EV_SET(&ke, fd, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
        kevent(state->kqfd, &ke, 1, nullptr, 0, nullptr);
    }
}

static int aeApiPoll(aeEventLoop *eventLoop, struct timeval *tvp) {
    auto *state = (aeApiState *)eventLoop->apidata;
    int retval, numevents = 0;

    if (tvp != nullptr) {
        struct timespec timeout;
        timeout.tv_sec = tvp->tv_sec;
        timeout.tv_nsec = tvp->tv_usec * 1000;
        retval = kevent(state->kqfd, nullptr, 0, state->events, eventLoop->setsize,
                        &timeout);
    } else {
        retval = kevent(state->kqfd, nullptr, 0, state->events, eventLoop->setsize,
                        nullptr);
    }

    if (retval > 0) {
        int j;

        numevents = retval;
        for(j = 0; j < numevents; j++) {
            int mask = 0;
            struct kevent *e = state->events+j;

            if (e->filter == EVFILT_READ) mask |= AE_READABLE;
            if (e->filter == EVFILT_WRITE) mask |= AE_WRITABLE;
            eventLoop->fired[j].fd = e->ident;
            eventLoop->fired[j].mask = mask;
        }
    }
    return numevents;
}

const char *aeApiName() {
    return "kqueue";
}
