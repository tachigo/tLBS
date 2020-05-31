//
// Created by 刘立悟 on 2020/5/31.
//

#include "threadpool_c.h"
#include "log.h"

using namespace tLBS;

ThreadPool::ThreadPool(const char *group, int maxThreadNum, int maxTaskNum) {
    this->shutdown = false;
    this->group = group;
    this->maxThreadNum = maxThreadNum;
    this->maxTaskNum = maxTaskNum;
    this->queueSize = 0;
    // 任务线程数组开空间
    this->execThreads = (pthread_t *) malloc(sizeof(pthread_t) * maxThreadNum);
    memset(this->execThreads, 0, sizeof(pthread_t) * maxThreadNum);
    // 队列开空间
    this->taskQueue = (ThreadPoolTask *) malloc(sizeof(ThreadPoolTask) * maxTaskNum);
    memset(this->taskQueue, 0, sizeof(ThreadPoolTask) * maxTaskNum);
    // 初始化互斥量和条件变量
    pthread_mutex_init(&this->mutexLock, nullptr);
    pthread_cond_init(&this->queueNotEmpty, nullptr);
    pthread_cond_init(&this->queueNotFull, nullptr);

    // 创建任务工作线程
    for (int i = 0; i < maxThreadNum; i++) {
        pthread_create(&this->execThreads[i], nullptr, execute, (void *) this);
        info("线程池(" + this->group + ")创建线程#")
            << (i + 1) << " [" << this->execThreads[i] << "]";
    }
    // 创建管程
    this->adminThread = nullptr;
    pthread_create(&this->adminThread, nullptr, admin, (void *)this);
}

ThreadPool::~ThreadPool() {
    free(this->execThreads);
    free(this->taskQueue);
}

pthread_mutex_t * ThreadPool::getMutexLock() {
    return &this->mutexLock;
}

pthread_cond_t * ThreadPool::getQueueNotFullCond() {
    return &this->queueNotFull;
}

pthread_cond_t * ThreadPool::getQueueNotEmptyCond() {
    return &this->queueNotEmpty;
}

bool ThreadPool::isShutdown() {
    return this->shutdown;
}

int ThreadPool::getQueueSize() {
    return this->queueSize;
}

std::string ThreadPool::getGroup() {
    return this->group;
}

void * ThreadPool::execute(void *threadPool) {
    auto *pool = (ThreadPool *)threadPool;
    // 锁住线程池
    pthread_mutex_lock(pool->getMutexLock());
    while (pool->getQueueSize() == 0 && !pool->isShutdown()) {
        // 当线程池无任务时，阻塞等待队列不空
        info("线程池(" + pool->getGroup() + ")队列为空,线程#")
                <<  " [" << pthread_self() << "]等待队列不为空";
        pthread_cond_wait(pool->getQueueNotEmptyCond(), pool->getMutexLock());
    }

    // 解锁线程池
    pthread_mutex_unlock(pool->getMutexLock());
    return (void *)1;
}

void * ThreadPool::admin(void *threadPool) {
    auto *pool = (ThreadPool *)threadPool;
    return (void *)1;
}