//
// Created by 刘立悟 on 2020/6/11.
//

#ifndef TLBS_EL_KQUEUE_H
#define TLBS_EL_KQUEUE_H

#include <sys/event.h>
#include <sys/time.h>
#include <string>

namespace tLBS {
    class EventLoop;

    class EventLoopKQueue {
    private:
        int kqfd;
        struct kevent *events;
    public:
        explicit EventLoopKQueue(EventLoop *el);
        ~EventLoopKQueue();

        int resize(int setSize);
        int addEvent(EventLoop *el, int fd, int flags);
        int delEvent(EventLoop *el, int fd, int flags);
        int poll(EventLoop *el, struct timeval *tvp);

        static std::string getName();
    };
}

#endif //TLBS_EL_KQUEUE_H
