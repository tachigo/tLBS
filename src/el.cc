//
// Created by liuliwu on 2020-05-28.
//

#include "el.h"
#include "log.h"

using namespace tLBS;


// 根据系统包含多路复用的层 且根据性能进行降序
#ifdef HAVE_EVPORT
//    #include "el_evport.cc"
#else
    #ifdef HAVE_EPOLL
//        #include "el_epoll.cc"
    #else
        #ifdef HAVE_KQUEUE
//            #include "el_kqueue.cc"
        #else
            #include "el_select.cc"
        #endif
    #endif
#endif

EventLoop *EventLoop::instance = nullptr;

EventLoop * EventLoop::create(int setSize) {
    if (instance == nullptr) {
        instance = new EventLoop(setSize);
    }
    return instance;
}

EventLoop * EventLoop::getInstance() {
    return instance;
}

EventLoop::EventLoop(int setSize) {
    this->setSize = setSize;
    this->lastTime = time(nullptr);
    this->stop = false;
    this->maxFd = -1;
    this->flags = 0;
    this->teHead = nullptr;
    this->teNextId = 0;
    int j;
    this->events = (FileEvent *) malloc(sizeof(FileEvent) * setSize);
    this->fired = (FiredEvent *) malloc(sizeof(FiredEvent) * setSize);
    for (j = 0; j < setSize; j++) {
        this->events[j] = FileEvent();
        this->events[j].flags = EL_NONE;

        this->fired[j] = FiredEvent();
    }
    this->handler = new EventLoopHandler();
}

FileEvent* EventLoop::getEvent(int j) {
    return &this->events[j];
}

void EventLoop::free() {
    delete instance;
}

EventLoop::~EventLoop() {
    this->handler = nullptr;
    ::free(this->events);
    ::free(this->fired);
}

int EventLoop::getSetSize() {
    return this->setSize;
}

EventLoopHandler *EventLoop::getHandler() {
    return this->handler;
}

void EventLoop::start() {
    this->stop = false;
    while (!this->stop) {
        this->processEvents(EL_ALL_EVENT|EL_CALL_AFTER_SLEEP);
    }
}

TimeEvent* EventLoop::searchEarliestTimeEvent() {
    TimeEvent *te = this->teHead;
    TimeEvent *nearest = nullptr;
    while (te) {
        if (!nearest || te->whenSec < nearest->whenSec ||
                (te->whenSec == nearest->whenSec && te->whenMs < nearest->whenMs)) {
            nearest = te;
        }
        te = te->next;
    }
    return nearest;
}


int EventLoop::processTimeEvents() {
//    info("处理事件事件");
    int processed = 0;
    time_t now = time(nullptr);
    this->lastTime = now;

    TimeEvent *te = this->teHead;
    long long maxId = this->teNextId - 1;
    while (te) {
        long nowSec, nowMs;
        if (te->id == EL_DELETED_EVENT_ID) {
            // 从链表中删除一个节点
            TimeEvent *nextTe = te->next;
            if (te->prev) {
                te->prev->next = te->next;
            }
            else {
                this->teHead = te->next;
            }
            if (te->next) {
                te->next->prev = te->prev;
            }
            delete te;
            te = nextTe;
            continue;
        }
        if (te->id > maxId) {
            te = te->next;
            continue;
        }
        getTimeval(&nowSec, &nowMs);
        if (nowSec > te->whenSec ||
                (nowSec == te->whenSec && nowMs >= te->whenMs)) {
            int ret = te->timeFallback(this, te->id, te->data);
            processed++;
            if (ret != EL_NO_MORE) {
                addMillisecondsToNow(ret, &te->whenSec, &te->whenMs);
            }
            else {
                te->id = EL_DELETED_EVENT_ID;
            }
        }
        te = te->next;
    }
    return processed;
}


int EventLoop::processEvents(int flags) {
    int processed = 0;
    int numEvents;
    if (!(flags & EL_TIME_EVENT) && !(flags & EL_FILE_EVENT)) {
        // 很奇怪 即不是文件事件 也不是定时任务事件 想干嘛
        return processed;
    }
    if (this->maxFd != -1 ||
            ((flags & EL_TIME_EVENT) && !(flags & EL_NOT_WAIT))) {
        // 没有文件事件的话，看看有没有定时任务事件能够执行
        int j;
        struct timeval tv = {0, 0};
        struct timeval *tvp;
        TimeEvent *earliest = nullptr; // 最早的一个定时任务事件
        if (flags & EL_TIME_EVENT && !(flags & EL_NOT_WAIT)) {
            // 如果是要执行定时任务事件
            earliest = this->searchEarliestTimeEvent();
        }
        if (earliest) {
            long nowSec, nowMs;
            getTimeval(&nowSec, &nowMs);
            tvp = &tv;

            long long ms = (earliest->whenSec - nowSec) * 1000 +
                    earliest->whenMs - nowMs;
            if (ms > 0) {
                // 如果是滞后事件
                tvp->tv_sec = ms / 1000;
                tvp->tv_usec = (ms % 1000) * 1000;
            }
            else {
                tvp->tv_sec = tvp->tv_usec = 0;
            }
        }
        else {
            if (flags & EL_NOT_WAIT) {
                tv.tv_sec = tv.tv_usec = 0;
                tvp = &tv;
            }
            else {
                tvp = nullptr;
            }
        }

        if (this->flags & EL_NOT_WAIT) {
            tv.tv_sec = tv.tv_usec = 0;
            tvp = &tv;
        }

        numEvents = this->getHandler()->poll(this, tvp);
        // 上一步如果阻塞进入sleep，需要做一些清理工作
        if (flags & EL_CALL_AFTER_SLEEP) {
            // todo
        }
        for (j = 0; j < numEvents; j++) {
            int fd = this->fired[j].fd;
            int firedFlags = this->fired[j].flags;
            FileEvent *fe = &this->events[fd];
            int fired = 0;

            // 正常情况下 先执行读，再执行写
            // 但是 invert 为真的情况下 则需要反过来
            int invert = fe->flags & EL_BARRIER;
            if (!invert && fe->flags & firedFlags & EL_READABLE) {
                // 处理事件
                info("处理socket读fd#") << fd;
                fe->rFallback(this, fd, firedFlags, fe->data);
                fired++;
                fe = &this->events[fd];
            }

            if (fe->flags & firedFlags & EL_WRITABLE) {
                if (!fired || fe->rFallback != fe->wFallback) {
                    info("处理socket写fd#") << fd;
                    fe->wFallback(this, fd, firedFlags, fe->data);
                    fired++;
                }
            }

            if (invert) {
                fe = &this->events[fd];
                if ((fe->flags & firedFlags & EL_READABLE) &&
                        (!fired || fe->wFallback != fe->rFallback)) {
                    info("处理socket读fd#") << fd;
                    fe->rFallback(this, fd, firedFlags, fe->data);
                    fired++;
                }
            }
            processed++;
        }
    }
    // 最后检查是否能处理定时任务事件
    if (flags & EL_TIME_EVENT) {
        processed += this->processTimeEvents();
    }
    return processed;
}

void EventLoop::setStop() {
    this->stop = true;
}

bool EventLoop::isStop() {
    return this->stop;
}

std::string EventLoop::getName() {
    return this->handler->getName();
}

long long EventLoop::addTimeEvent(long long milliseconds, elTimeFallback timeFallback, void *data) {
    long long id = this->teNextId++;
    this->teHead = new TimeEvent(id, milliseconds, timeFallback, data, nullptr, this->teHead);
    return id;
}

int EventLoop::delTimeEvent(long long id) {
    // 走链表删除指定节点
    TimeEvent *te = this->teHead;
    while (te) {
        if (te->id == id) {
            te->id = EL_DELETED_EVENT_ID;
            return EL_OK;
        }
        te = te->next;
    }
    return EL_ERR;
}

void EventLoop::delFileEvent(int fd, int flags) {
    if (fd >= this->setSize) {
        return;
    }
    FileEvent *fe = &this->events[fd];
    if (fe->flags == EL_NONE) {
        return;
    }
    if (flags & EL_WRITABLE) {
        flags |= EL_BARRIER;
    }
    this->getHandler()->delEvent(fd, flags);
    fe->flags = fe->flags & (~flags);
    if (fd == this->maxFd && fe->flags == EL_NONE) {
        int j;
        for (j = this->maxFd - 1; j >= 0; j--) {
            if (this->events[j].flags != EL_NONE) {
                break;
            }
        }
        this->maxFd = j;
    }
}

int EventLoop::addFileEvent(int fd, int flags, elFileFallback proc, void *data) {
    if (fd >= this->setSize) {
        errno = ERANGE;
        return EL_ERR;
    }
    FileEvent *fe = &this->events[fd];
    if (this->getHandler()->addEvent(fd, flags) == -1) {
        return EL_ERR;
    }
    fe->flags |= flags;
    if (flags & EL_READABLE) {
        fe->rFallback = proc;
    }
    if (flags & EL_WRITABLE) {
        fe->wFallback = proc;
    }
    fe->data = data;
    if (fd > this->maxFd) {
        this->maxFd = fd;
    }
    return EL_OK;
}


int EventLoop::getMaxFd() {
    return this->maxFd;
}

void EventLoop::addFiredEvent(int key, int fd, int flags) {
    this->fired[key].fd = fd;
    this->fired[key].flags = flags;
}