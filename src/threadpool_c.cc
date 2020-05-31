//
// Created by 刘立悟 on 2020/5/31.
//


#include "threadpool_c.h"
#include "log.h"
#include "common.h"

using namespace tLBS;


std::map<std::string, ThreadPool *> ThreadPool::map;

ThreadPool * ThreadPool::createPool(std::string group, int minThreadNum, int maxThreadNum, int maxTaskNum) {
    auto mapIter = map.find(group);
    if (mapIter == map.end()) {
        // 没找到
        map[group] = new ThreadPool(group, minThreadNum, maxThreadNum, maxTaskNum);
        // 在进程栈内存上创建对象保存在这个静态map中有问题，所以使用在堆内存上创建的对象
    }
    return map[group];
}

ThreadPool * ThreadPool::getPool(std::string group) {
    auto mapIter = map.find(group);
    if (mapIter != map.end()) {
        // 找到了
        return map[group];
    }
    else {
        return nullptr;
    }
}

void ThreadPool::destroyPools() {
    info("销毁所有的thread pool对象");
    for (auto mapIter = map.begin(); mapIter != map.end(); mapIter++) {
        std::string group = mapIter->first;
        delete map[group];
//        map.erase(group);
    }
}


ThreadPool::ThreadPool(std::string group, int minThreadNum, int maxThreadNum, int maxTaskNum) {
    info("创建一个名为(" + group + ")的线程池")
        << ", 最小线程数: " << minThreadNum
        << ", 最大线程数: " << maxThreadNum
        << ", 任务队列最大长度: " << maxTaskNum;
    this->shutdown = false;
    this->group = group;
    this->minThreadNum = minThreadNum;
    this->maxThreadNum = maxThreadNum;
    this->busyThreadNum = 0;
    this->liveThreadNum = minThreadNum;
    this->maxTaskNum = maxTaskNum;
    this->queueSize = 0;
    this->queueFront = 0;
    this->queueRear = 0;
    // 任务线程数组开空间
    this->execThreads = (pthread_t *) malloc(sizeof(pthread_t) * maxThreadNum);
    memset(this->execThreads, 0, sizeof(pthread_t) * maxThreadNum);
    // 队列开空间
    this->taskQueue = (ThreadPoolTask *) malloc(sizeof(ThreadPoolTask) * maxTaskNum);
    memset(this->taskQueue, 0, sizeof(ThreadPoolTask) * maxTaskNum);
    // 初始化互斥量和条件变量
    pthread_mutex_init(&this->mutexLock, nullptr);
    pthread_mutex_init(&this->counterLock, nullptr);
    pthread_cond_init(&this->queueNotEmpty, nullptr);
    pthread_cond_init(&this->queueNotFull, nullptr);

    // 创建任务工作线程
    for (int i = 0; i < minThreadNum; i++) {
        pthread_create(&this->execThreads[i], nullptr, execute, (void *) this);
//        info("线程池(" + this->group + ")创建线程#")
//            << (i + 1) << " [" << this->execThreads[i] << "]";
    }
    // 创建管程
    this->adminThread = nullptr;
    pthread_create(&this->adminThread, nullptr, admin, (void *)this);
}

int ThreadPool::getMinThreadNum() {
    return this->minThreadNum;
}

int ThreadPool::getLiveThreadNum() {
    return this->liveThreadNum;
}

ThreadPool::~ThreadPool() {
    free(this->execThreads);
    free(this->taskQueue);
}

void ThreadPool::lockMutex() {
    pthread_mutex_lock(&this->mutexLock);
}

void ThreadPool::unlockMutex() {
    pthread_mutex_unlock(&this->mutexLock);
}

void ThreadPool::lockCounter() {
    pthread_mutex_lock(&this->counterLock);
}

void ThreadPool::unlockCounter() {
    pthread_mutex_unlock(&this->counterLock);
}

void ThreadPool::incrBusyThreadNum(int incr) {
    this->busyThreadNum += incr;
}

void ThreadPool::decrBusyThreadNum(int decr) {
    this->busyThreadNum -= decr;
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

int ThreadPool::enqueueTask(void *(*fn)(void *), void *arg) {
    // 锁住线程池
    pthread_mutex_lock(&this->mutexLock);
    // 如果任务队列满了，则阻塞等待队列不满
    while ((this->queueSize == this->maxTaskNum) && !this->shutdown) {
        info("线程池(" + this->group + ")任务队列满，等待队列不满信号");
        pthread_cond_wait(&this->queueNotFull, &this->mutexLock);
    }
    // 线程池关闭
    if (this->shutdown) {
        pthread_mutex_unlock(&this->mutexLock);
        return C_ERR;
    }

    this->taskQueue[this->queueRear].fn = fn;
    this->taskQueue[this->queueRear].arg = arg;

    this->queueRear = (this->queueRear + 1) % this->maxTaskNum;
    this->queueSize += 1;
    info("线程池(" + this->group + ")任务队列长度: ") << this->queueSize;

    // 发出队列不空条件通知
    pthread_cond_signal(&this->queueNotEmpty);
    // 解锁线程池
    pthread_mutex_unlock(&this->mutexLock);
    return C_OK;
}


int ThreadPool::dequeueTask(ThreadPoolTask *task) {
    // 锁住线程池
    pthread_mutex_lock(&this->mutexLock);
    // 如果任务队列空了，则阻塞等待队列不空
    while ((this->queueSize == 0) && !this->shutdown) {
        pthread_cond_wait(&this->queueNotEmpty, &this->mutexLock);
    }
    // 线程池关闭
    if (this->shutdown) {
        pthread_mutex_unlock(&this->mutexLock);
        return C_ERR;
    }

    task->fn = this->taskQueue[this->queueFront].fn;
    task->arg = this->taskQueue[this->queueFront].arg;

    this->queueFront = (this->queueFront + 1) % this->maxTaskNum;
    this->queueSize -= 1;
    // 发出队列不满条件通知
    pthread_cond_signal(&this->queueNotFull);
    // 解锁线程池
    pthread_mutex_unlock(&this->mutexLock);
    return C_OK;
}

void * ThreadPool::execute(void *threadPool) {
    auto *pool = (ThreadPool *)threadPool;

    while (true) {
        auto *task = (ThreadPoolTask *) malloc(sizeof(ThreadPoolTask));

        if (pool->dequeueTask(task) == C_ERR) {
            // 任务出队失败
            return (void *)1;
        }

        pool->lockCounter();
        pool->incrBusyThreadNum(1);
        pool->unlockCounter();

        // 执行任务
        (*(task->fn))(task->arg);

        pool->lockCounter();
        pool->decrBusyThreadNum(1);
        pool->unlockCounter();
    }

    return (void *) 0; // never reached
}


int ThreadPool::getBusyThreadNum() {
    return this->busyThreadNum;
}

void * ThreadPool::admin(void *threadPool) {
    auto *pool = (ThreadPool *)threadPool;

    while(!pool->isShutdown()) {
        sleep(1); // 每隔1秒检查一次
        pool->lockCounter();
//        info("线程池(" + pool->getGroup() + ")")
//            << "繁忙线程数: " << pool->getBusyThreadNum()
//            << ", 存活线程数: " << pool->getLiveThreadNum();
        int busyThreadNum = pool->getBusyThreadNum();
        int liveThreadNum = pool->getLiveThreadNum();
        pool->unlockCounter();


    }
    return (void *)1;
}