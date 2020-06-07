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
        const char *name;
        void *arg; // 参数
        void *(* fn)(void *arg); // 方法
        ThreadPoolTask *next;
    };

    // 线程池
    class ThreadPool {
    private:
        static std::map<std::string, ThreadPool *>map;
        std::string group; // 线程池所属的组
        pthread_mutex_t mutexLock; // 互斥量
        pthread_mutex_t counterLock; // 计数器互斥量
        pthread_cond_t queueCond;
        bool shutdown; // 线程池是否是关闭状态
        std::string info;

        pthread_t *execThreads; // 存放执行线程的id的数组
        int threadNum; // 线程数

        ThreadPoolTask *taskHead; // 任务队列，这里应该是一个循环队列
        int queueSize;
        ThreadPool(std::string group, int threadNum);
    public:
        static ThreadPool *getPool(std::string group);
        static ThreadPool *createPool(std::string group, int threadNum);
        static void free();
        ~ThreadPool();
        int enqueueTask(void *(*fn)(void *arg), void *arg, const char *name);
        ThreadPoolTask *dequeueTask();

        std::string getGroup();
        std::string getInfo();

        static void *execute(void *threadPool);
    };
}

#endif //TLBS_THREADPOOL_C_H
