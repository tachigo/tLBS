//
// Created by liuliwu on 2020-05-29.
//

#ifndef TLBS_EL_SELECT_H
#define TLBS_EL_SELECT_H

#include <sys/select.h>
#include <string>



namespace tLBS {

    class EventLoop; // 提前声明一下

    class EventLoopSelect {
    private:
        fd_set readFds;
        fd_set writeFds;

        fd_set _readFds;
        fd_set _writeFds;

    public:
        EventLoopSelect();
        ~EventLoopSelect();

        int resize(int setSize);

        int addEvent(int fd, int flags);

        int delEvent(int fd, int flags);


        int poll(EventLoop *el, struct timeval *tvp);

        static std::string getName();
    };
}

#endif //TLBS_EL_SELECT_H
