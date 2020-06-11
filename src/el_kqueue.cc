//
// Created by 刘立悟 on 2020/6/11.
//

#include "el_kqueue.h"
#include "log.h"
#include "common.h"

using namespace tLBS;

EventLoopKQueue::EventLoopKQueue(EventLoop *el) {
    this->events = (struct kevent *)malloc(sizeof(struct kevent)*el->getSetSize());
    this->kqfd = kqueue();
    if (this->kqfd == -1) {
        ::free(this->events);
        fatal("kqueue failed!");
    }
}

EventLoopKQueue::~EventLoopKQueue() {
    close(this->kqfd);
    ::free(this->events);
}


int EventLoopKQueue::resize(int setSize) {
    this->events = (struct kevent *)realloc(this->events, sizeof(struct kevent)*setSize);
    return 0;
}

int EventLoopKQueue::addEvent(EventLoop *el, int fd, int flags) {
    UNUSED(el);
    struct kevent ke;


    if ((flags & EL_READABLE)) {
        EV_SET(&ke, fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
        if (kevent(this->kqfd, &ke, 1, nullptr, 0, nullptr) == -1) return -1;
    }
    if (flags & EL_WRITABLE) {
        EV_SET(&ke, fd, EVFILT_WRITE, EV_ADD, 0, 0, NULL);
        if (kevent(this->kqfd, &ke, 1, nullptr, 0, nullptr) == -1) return -1;
    }
    return 0;
}


int EventLoopKQueue::delEvent(EventLoop *el, int fd, int flags) {
    UNUSED(el);
    struct kevent ke;

    if (flags & EL_READABLE) {
        EV_SET(&ke, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
        kevent(this->kqfd, &ke, 1, nullptr, 0, nullptr);
    }
    if (flags & EL_WRITABLE) {
        EV_SET(&ke, fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
        kevent(this->kqfd, &ke, 1, nullptr, 0, nullptr);
    }
    return 0;
}


int EventLoopKQueue::poll(EventLoop *el, struct timeval *tvp) {
    int retVal, numEvents = 0;

    if (tvp != nullptr) {
        struct timespec timeout;
        timeout.tv_sec = tvp->tv_sec;
        timeout.tv_nsec = tvp->tv_usec * 1000;
        retVal = kevent(this->kqfd, nullptr, 0, this->events, el->getSetSize(),
                        &timeout);
    } else {
        retVal = kevent(this->kqfd, nullptr, 0, this->events, el->getSetSize(),
                        nullptr);
    }

    if (retVal > 0) {
        int j;

        numEvents = retVal;
        for(j = 0; j < numEvents; j++) {
            int flags = 0;
            struct kevent *e = this->events+j;

            if (e->filter == EVFILT_READ) flags |= EL_READABLE;
            if (e->filter == EVFILT_WRITE) flags |= EL_WRITABLE;
            el->addFiredEvent(j, e->ident, flags);
        }
    }
    return numEvents;
}


std::string EventLoopKQueue::getName() {
    return "kqueue";
}