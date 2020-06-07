//
// Created by 刘立悟 on 2020/6/7.
//

#include "el_epoll.h"
#include "log.h"

using namespace tLBS;

EventLoopEPoll::EventLoopEPoll(EventLoop *el) {
    this->events = (epoll_event *)malloc(sizeof(struct epoll_event)*el->getSetSize());
    this->epfd = epoll_create(1024); /* 1024 is just a hint for the kernel */
    if (this->epfd == -1) {
        ::free(this->events);
        fatal("epoll_create failed!");
    }
}

EventLoopEPoll::~EventLoopEPoll() {
    close(this->epfd);
    ::free(this->events);
}


int EventLoopEPoll::resize(int setSize) {
    this->events = (epoll_event *)realloc(this->events, sizeof(struct epoll_event)*setSize);
    return 0;
}


int EventLoopEPoll::addEvent(EventLoop *el, int fd, int flags) {
    struct epoll_event ee = {0}; /* avoid valgrind warning */
    /* If the fd was already monitored for some event, we need a MOD
     * operation. Otherwise we need an ADD operation. */
    int op = el->getEvent(fd)->flags == EL_NONE ?
             EPOLL_CTL_ADD : EPOLL_CTL_MOD;

    ee.events = 0;
    flags |= el->getEvent(fd)->flags; /* Merge old events */
    if (flags & EL_READABLE) ee.events |= EPOLLIN;
    if (flags & EL_WRITABLE) ee.events |= EPOLLOUT;
    ee.data.fd = fd;
    if (epoll_ctl(this->epfd,op,fd,&ee) == -1) {
        return -1;
    }
    return 0;
}

int EventLoopEPoll::delEvent(EventLoop *el, int fd, int delFlags) {
    struct epoll_event ee = {0}; /* avoid valgrind warning */
    int flags = el->getEvent(fd)->flags & (~delFlags);

    ee.events = 0;
    if (flags & EL_READABLE) ee.events |= EPOLLIN;
    if (flags & EL_WRITABLE) ee.events |= EPOLLOUT;
    ee.data.fd = fd;
    if (flags != EL_NONE) {
        epoll_ctl(this->epfd, EPOLL_CTL_MOD, fd, &ee);
    } else {
        /* Note, Kernel < 2.6.9 requires a non null event pointer even for
         * EPOLL_CTL_DEL. */
        epoll_ctl(this->epfd, EPOLL_CTL_DEL, fd, &ee);
    }
}

int EventLoopEPoll::poll(EventLoop *el, struct timeval *tvp) {
    int retVal, numEvents = 0;

    retVal = epoll_wait(this->epfd,this->events,el->getSetSize(),
                        tvp ? (tvp->tv_sec*1000 + tvp->tv_usec/1000) : -1);
    if (retVal > 0) {
        int j;

        numEvents = retVal;
        for (j = 0; j < numEvents; j++) {
            int flags = 0;
            struct epoll_event *e = this->events+j;

            if (e->events & EPOLLIN) flags |= EL_READABLE;
            if (e->events & EPOLLOUT) flags |= EL_WRITABLE;
            if (e->events & EPOLLERR) flags |= EL_WRITABLE | EL_READABLE;
            if (e->events & EPOLLHUP) flags |= EL_WRITABLE | EL_READABLE;
            el->addFiredEvent(j, e->data.fd, flags);
        }
    }
    return numEvents;
}


std::string EventLoopEPoll::getName()  {
    return "epoll";
}