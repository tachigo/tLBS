//
// Created by liuliwu on 2020-05-28.
//

#ifndef TLBS_EL_H
#define TLBS_EL_H

#include "common.h"
#include <string>

#define EL_OK 0
#define EL_ERR -1

// 没有注册的事件
#define EL_NONE 0
// 当文件描述符可读的时候发送事件
#define EL_READABLE 1
// 当文件描述符可写的时候发送事件
#define EL_WRITABLE (1<<1)
// 如果可读事件在事件循环中发送了，那么不发送可写事件
#define EL_BARRIER (1<<2)


// 文件事件 例如tcp的套接字文件描述符
#define EL_FILE_EVENT 1
// 定时任务事件
#define EL_TIME_EVENT (1<<1)
// 所有事件
#define EL_ALL_EVENT (EL_FILE_EVENT | EL_TIME_EVENT)
// 不等待的事件
#define EL_NOT_WAIT (1<<2)
// sleep后调用的事件
#define EL_CALL_AFTER_SLEEP (1<<3)

// 不重复执行
#define EL_NO_MORE -1
// 标记一个事件是删除了的
#define EL_DELETED_EVENT_ID -1
// max_clients + FD_SET_INCR
#define FD_SET_INCR (MIN_REVERSED_FDS + 96)

// 根据系统包含多路复用的层 且根据性能进行降序
// 统一命名别名为 EventLoopHandler
#ifdef HAVE_EVPORT
//    #include "el_evport.h"
//    #define EventLoopHandler EventLoopEvport
#else
    #ifdef HAVE_EPOLL
        #include "el_epoll.h"
        #define EventLoopHandler EventLoopEPoll
    #else
        #ifdef HAVE_KQUEUE
//            #include "el_kqueue.h"
//            #define EventLoopHandler EventLoopKqueue
        #else
            #include "el_select.h"
            #define EventLoopHandler EventLoopSelect
        #endif
    #endif
#endif



namespace tLBS {

    typedef void elFileFallback (int fd, int flags, void *data);
    typedef int elTimeFallback (long long id, void *data);
    typedef void elBeforeSleepFallback ();

    class FiredEvent {
    public:
        int fd; // 文件描述符
        int flags; // 标记
    };

    class FileEvent {
    public:
        int flags;
//        elFileFallback *elFallback;
        elFileFallback *rFallback; // 读回调
        elFileFallback *wFallback; // 写回调
        void *data; // 数据
    };

    class TimeEvent {
    public:
        long long id;
        long whenSec;
        long whenMs;
        elTimeFallback *timeFallback; // 定时任务
        void *data;
        TimeEvent *prev;
        TimeEvent *next;

        TimeEvent(long long id, long long milliseconds, elTimeFallback *timeFallback, void *data, TimeEvent *prev, TimeEvent *next) {
            this->id = id;
            addMillisecondsToNow(milliseconds, &this->whenSec, &this->whenMs);
            this->timeFallback = timeFallback;
            this->data = data;
            this->prev = prev;
            this->next = next;
            if (this->next) {
                this->next->prev = this;
            }
        }
    };

    class EventLoop {
    private:
        static EventLoop* instance;
        int maxFd; // 当前注册了的最大的事件数
        int setSize; // 文件描述符集的大小
        bool stop; // 循环是否停止
        int flags; // 标记
        time_t lastTime; // 记录时钟
        FileEvent *events;
        FiredEvent *fired;
        TimeEvent *teHead; // 定时任务事件链表头结点
        long long teNextId; // 定时任务事件ID计数器
        elBeforeSleepFallback *beforeSleep;

        EventLoopHandler *handler;
        TimeEvent *searchEarliestTimeEvent(); // 搜索最早的定时任务事件
        int processEvents(int flags);
        int processTimeEvents();
        explicit EventLoop(int setSize);
    public:
        static EventLoop *create(int setSize);
        static EventLoop *getInstance();
        static void free();
        ~EventLoop();
        EventLoopHandler *getHandler();
        std::string getName();
        int getMaxFd();
        int getSetSize();
        void start();
        void setStop();
        bool isStop();
        long long addTimeEvent(long long milliseconds, elTimeFallback timeFallback, void *data);
        int delTimeEvent(long long id);
        int addFileEvent(int fd, int flags, elFileFallback proc, void *data);
        void delFileEvent(int fd, int flags);
        FileEvent *getEvent(int j);
        void addFiredEvent(int key, int fd, int flags);

        void setBeforeSleep(elBeforeSleepFallback beforeSleepFallback);

        bool fdRegistered(int fd);
        bool fdReadable(int fd);
        bool fdWritable(int fd);
    };
}

#endif //TLBS_EL_H
