//
// Created by 刘立悟 on 2020/5/31.
//

#ifndef TLBS_THREADPOOL_C_H
#define TLBS_THREADPOOL_C_H

// c风格的线程池

#include <pthread.h>
#include <string>

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
        std::string group; // 线程池所属的组
        pthread_mutex_t mutexLock; // 互斥量
        pthread_cond_t queueNotFull; // 任务队列非满
        pthread_cond_t queueNotEmpty; // 任务队列非空
        bool shutdown; // 线程池是否是关闭状态

        pthread_t *execThreads; // 存放执行线程的id的数组
        int maxThreadNum; // 最大线程数
        pthread_t adminThread; // 管程id

        ThreadPoolTask *taskQueue; // 任务队列，这里应该是一个循环队列
        int maxTaskNum; // 最大任务数
        int queueSize; // 队列大小
    public:

        ThreadPool(const char *group, int maxThreadNum, int maxTaskNum);
        ~ThreadPool();
        pthread_mutex_t *getMutexLock();
        pthread_cond_t *getQueueNotFullCond();
        pthread_cond_t *getQueueNotEmptyCond();
        bool isShutdown();
        int getQueueSize();
        std::string getGroup();

        static void *execute(void *threadPool);
        static void *admin(void *threadPool);
    };
}

#endif //TLBS_THREADPOOL_C_H
