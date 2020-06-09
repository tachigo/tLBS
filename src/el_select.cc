//
// Created by liuliwu on 2020-05-28.
//

#include "el_select.h"

using namespace tLBS;


EventLoopSelect::EventLoopSelect(EventLoop *el) {
    UNUSED(el);
    this->readFds = *((fd_set *) malloc(sizeof(fd_set)));
    this->writeFds = *((fd_set *) malloc(sizeof(fd_set)));
    FD_ZERO(&this->readFds);
    FD_ZERO(&this->writeFds);
}

EventLoopSelect::~EventLoopSelect() {
    free(&this->readFds);
    free(&this->writeFds);
}

bool EventLoopSelect::fdRegistered(int fd) {
    return fdReadable(fd) || fdWritable(fd);
}

bool EventLoopSelect::fdReadable(int fd) {
    return FD_ISSET(fd, &this->readFds);
}

bool EventLoopSelect::fdWritable(int fd) {
    return FD_ISSET(fd, &this->writeFds);
}

int EventLoopSelect::resize(int setSize) {
    if (setSize >= FD_SETSIZE) {
        return -1;
    }
    return 0;
}

int EventLoopSelect::addEvent(EventLoop *el, int fd, int flags) {
//    if (fd > FD_SETSIZE) {
//        fatal("select fd_set fd[") << fd << "] > FD_SETSIZE[" << FD_SETSIZE << "]";
//    }
    UNUSED(el);
    if (flags & EL_READABLE && !FD_ISSET(fd, &this->readFds)) {
        FD_SET(fd, &this->readFds);
    }
    if (flags & EL_WRITABLE && !FD_ISSET(fd, &this->writeFds)) {
        FD_SET(fd, &this->writeFds);
    }
    return 0;
}

int EventLoopSelect::delEvent(EventLoop *el, int fd, int flags)  {
    UNUSED(el);
    if (flags & EL_READABLE && FD_ISSET(fd, &this->readFds)) {
        FD_CLR(fd, &this->readFds);
    }
    if (flags & EL_WRITABLE && FD_ISSET(fd, &this->writeFds)) {
        FD_CLR(fd, &this->writeFds);
    }
    return 0;
}

int EventLoopSelect::poll(EventLoop *el, struct timeval *tvp)  {
    int retVal, fd, feKey = 0;

    memcpy(&this->_readFds, &this->readFds, sizeof(fd_set));
    memcpy(&this->_writeFds, &this->writeFds, sizeof(fd_set));

    retVal = select(el->getMaxFd() + 1, &this->_readFds, &this->_writeFds, nullptr, tvp);
    if (retVal > 0) {
        for (fd = 0; fd <= el->getMaxFd(); fd++) {
            int flags = 0;
            FileEvent *fe = el->getEvent(fd);
            if (fe->flags == EL_NONE) {
                // 0,1,2,3,4,5
//                info(fd) << " EL_NONE";
                continue;
            }
            if (fe->flags & EL_READABLE && FD_ISSET(fd, &this->_readFds)) {
                flags |= EL_READABLE;
            }
            if (fe->flags & EL_WRITABLE && FD_ISSET(fd, &this->_writeFds)) {
                flags |= EL_WRITABLE;
            }
            el->addFiredEvent(feKey, fd, flags);
            feKey++;
        }
    }
    else if (retVal < 0) {
//        if (errno == EBADF) {
//
//        }
        error("select error: ") << strerror(errno) << "(" << errno << ")";
//        sleep(1);
//        info(el->getMaxFd());
    }
    else {
        info("select 0");
    }

    return feKey;
}

std::string EventLoopSelect::getName()  {
    return "select";
}