//
// Created by 刘立悟 on 2020/6/7.
//

#ifndef TLBS_EL_EPOLL_H
#define TLBS_EL_EPOLL_H

#include <sys/epoll.h>
#include <string>

namespace tLBS {
    class EventLoop;

    class EventLoopEPoll {
        int epfd;
        struct epoll_event *events;
    public:
        EventLoopEPoll(EventLoop *el);
        ~EventLoopEPoll();
        int resize(int setSize);

        int addEvent(EventLoop *el, int fd, int flags);

        int delEvent(EventLoop *el, int fd, int flags);


        int poll(EventLoop *el, struct timeval *tvp);

        static std::string getName();
    };
}


#endif //TLBS_EL_EPOLL_H
