//
// Created by liuliwu on 2020-05-28.
//

#ifndef TLBS_EL_H
#define TLBS_EL_H

#include <string>

#define EL_OK 0
#define EL_ERR -1

#define EL_NONE 0 // 没有注册的事件
#define EL_READABLE 1 // 当文件描述符可读的时候发送事件
#define EL_WRITABLE 2 // 当文件描述符可写的时候发送事件

// 根据系统包含多路复用的层 且根据性能进行降序
#ifdef HAVE_EVPORT
//    #include "el_evport.cc"
//    #define EventLoopHandler EventLoopEvPort
#else
    #ifdef HAVE_EPOLL
//        #include "el_epoll.cc"
//        #define EventLoopHandler EventLoopEPoll
    #else
        #ifdef HAVE_KQUEUE
//            #include "el_kqueue.cc"
//            #define EventLoopHandler EventLoopKQueue
        #else
            #include "el_select.cc"
            #define EventLoopHandler EventLoopSelect
        #endif
    #endif
#endif



namespace tLBS {

    class Event { // 事件类
    public:
        int fd; // 文件描述符
        int flags; // 标记
    };

    class EventLoop {
    private:
        int maxFd; // 当前注册了的最大的事件数
        int setSize; // 文件描述符集的大小
        bool stop; // 循环是否停止
        int flags; // 标记
        time_t lastTime; // 记录时钟

        EventLoopHandler *handler;

        int processEvents();
    public:
        explicit EventLoop(int setSize);
        ~EventLoop();
        EventLoopHandler *getHandler();
        std::string getName();
        int getSetSize();
        void start();
        void setStop();
        bool isStop();
    };
}

#endif //TLBS_EL_H
