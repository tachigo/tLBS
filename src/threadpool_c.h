//
// Created by 刘立悟 on 2020/5/31.
//

#ifndef TLBS_THREADPOOL_C_H
#define TLBS_THREADPOOL_C_H

// c风格的线程池

#include <pthread.h>
#include <string>
#include <map>

namespace tLBS {
    // 线程池任务
    class ThreadPoolTask {
    public:
        void *arg; // 参数
        void *(* fn)(void *arg); // 方法
    };

    // 线程池
    class ThreadPool {
    private:
        static std::map<std::string, ThreadPool *>map;
        std::string group; // 线程池所属的组
        pthread_mutex_t mutexLock; // 互斥量
        pthread_mutex_t counterLock; // 计数器互斥量
        pthread_cond_t queueNotFull; // 任务队列非满
        pthread_cond_t queueNotEmpty; // 任务队列非空
        bool shutdown; // 线程池是否是关闭状态

        pthread_t *execThreads; // 存放执行线程的id的数组
        int minThreadNum; // 最小线程数
        int maxThreadNum; // 最大线程数
        int busyThreadNum; // 忙线程数
        int liveThreadNum; // 存活的线程数
        pthread_t adminThread; // 管程id

        ThreadPoolTask *taskQueue; // 任务队列，这里应该是一个循环队列
        int maxTaskNum; // 最大任务数
        int queueSize; // 队列大小
        int queueFront; // 队列头指针
        int queueRear; // 队列尾指针
        ThreadPool(std::string group, int minThreadNum, int maxThreadNum, int maxTaskNum);
    public:
        static ThreadPool *getPool(std::string group);
        static ThreadPool *createPool(std::string group, int minThreadNum, int maxThreadNum, int maxTaskNum);
        static void destroyPools();
        ~ThreadPool();
        void lockMutex();
        void unlockMutex();

        void lockCounter();
        void unlockCounter();

        void incrBusyThreadNum(int incr);
        void decrBusyThreadNum(int decr);
        int getBusyThreadNum();

        int getMinThreadNum();
        int getLiveThreadNum();

        bool isShutdown();
        int getQueueSize();
        void incrQueueSize(int incr);
        void decrQueueSize(int decr);
        int enqueueTask(void *(*fn)(void *arg), void *arg);
        int dequeueTask(ThreadPoolTask *task);

        std::string getGroup();

        static void *execute(void *threadPool);
        static void *admin(void *threadPool);
    };
}

#endif //TLBS_THREADPOOL_C_H
