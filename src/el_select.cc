//
// Created by liuliwu on 2020-05-28.
//

#include "el_select.h"

using namespace tLBS;


EventLoopSelect::EventLoopSelect(EventLoop *el) {
    this->readFds = *((fd_set *) malloc(sizeof(fd_set)));
    this->writeFds = *((fd_set *) malloc(sizeof(fd_set)));
    FD_ZERO(&this->readFds);
    FD_ZERO(&this->writeFds);
}

EventLoopSelect::~EventLoopSelect() {
    free(&this->readFds);
    free(&this->writeFds);
}

int EventLoopSelect::resize(int setSize) {
    if (setSize >= FD_SETSIZE) {
        return -1;
    }
    return 0;
}

int EventLoopSelect::addEvent(EventLoop *el, int fd, int flags) {
    UNUSED(el);
    if (flags & EL_READABLE) {
        FD_SET(fd, &this->readFds);
    }
    if (flags & EL_WRITABLE) {
        FD_SET(fd, &this->writeFds);
    }
    return 0;
}

int EventLoopSelect::delEvent(EventLoop *el, int fd, int flags)  {
    UNUSED(el);
    if (flags & EL_READABLE) {
        FD_CLR(fd, &this->readFds);
    }
    if (flags & EL_WRITABLE) {
        FD_CLR(fd, &this->writeFds);
    }
    return 0;
}

int EventLoopSelect::poll(EventLoop *el, struct timeval *tvp)  {
    int retVal, j, numEvents = 0;

    memcpy(&this->_readFds, &this->readFds, sizeof(fd_set));
    memcpy(&this->_writeFds, &this->writeFds, sizeof(fd_set));

    retVal = select(el->getMaxFd() + 1, &this->_readFds, &this->_writeFds, nullptr, tvp);
    if (retVal > 0) {
        for (j = 0; j <= el->getMaxFd(); j++) {
            int flags = 0;
            FileEvent *fe = el->getEvent(j);
            if (fe->flags == EL_NONE) {
                continue;
            }
            if (fe->flags & EL_READABLE && FD_ISSET(j, &this->_readFds)) {
                flags |= EL_READABLE;
            }
            if (fe->flags & EL_WRITABLE && FD_ISSET(j, &this->_writeFds)) {
                flags |= EL_WRITABLE;
            }
            el->addFiredEvent(numEvents, j, flags);
            numEvents++;
        }
    }
    else {
//        sleep(1);
//        info(el->getMaxFd());
    }

    return numEvents;
}

std::string EventLoopSelect::getName()  {
    return "select";
}