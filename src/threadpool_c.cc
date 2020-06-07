//
// Created by 刘立悟 on 2020/5/31.
//


#include "threadpool_c.h"
#include "log.h"
#include "common.h"

using namespace tLBS;


std::map<std::string, ThreadPool *> ThreadPool::map;

ThreadPool * ThreadPool::createPool(std::string group, int threadNum) {
    auto mapIter = map.find(group);
    if (mapIter == map.end()) {
        // 没找到
        map[group] = new ThreadPool(group, threadNum);
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

void ThreadPool::free() {
    std::vector<ThreadPool *> freeList;
    for (auto mapIter = map.begin(); mapIter != map.end(); mapIter++) {
        std::string group = mapIter->first;
        freeList.push_back(mapIter->second);
    }
    for (int i = 0; i < freeList.size(); i++) {
        ThreadPool *threadPool = freeList[i];
        map.erase(threadPool->getGroup());
        delete threadPool;
    }
}


ThreadPool::ThreadPool(std::string group, int threadNum) {
    this->shutdown = false;
    this->group = group;
    this->threadNum = threadNum;
    this->queueSize = 0;
    // 任务线程数组开空间
    this->execThreads = (pthread_t *) malloc(sizeof(pthread_t) * threadNum);
    memset(this->execThreads, 0, sizeof(pthread_t) * threadNum);
    // 队列开空间
    this->taskHead = nullptr;
    // 初始化互斥量和条件变量
    pthread_mutex_init(&this->mutexLock, nullptr);
    pthread_mutex_init(&this->counterLock, nullptr);
    pthread_cond_init(&this->queueCond, nullptr);

    // 创建任务工作线程
    for (int i = 0; i < threadNum; i++) {
        pthread_create(&this->execThreads[i], nullptr, execute, (void *) this);
//        info("线程池(" + this->group + ")创建线程#")
//            << (i + 1) << " [" << this->execThreads[i] << "]";
    }
    char buf[100];
    snprintf(buf, sizeof(buf) - 1, "threadpool[%s](%d)", this->group.c_str(), this->threadNum);
    this->info = buf;
    warning("创建") << this->getInfo();
}


ThreadPool::~ThreadPool() {
    info("销毁") << this->getInfo();
    ::free(this->execThreads);
    ::free(this->taskHead);
}

std::string ThreadPool::getGroup() {
    return this->group;
}

std::string ThreadPool::getInfo() {
    return this->info;
}

int ThreadPool::enqueueTask(void *(*fn)(void *), void *arg, const char *name) {
    // 锁住线程池
    pthread_mutex_lock(&this->mutexLock);
    // 线程池关闭
    if (this->shutdown) {
        pthread_mutex_unlock(&this->mutexLock);
        return C_ERR;
    }

    auto *task = new ThreadPoolTask();
    task->name = name;
    task->arg = arg;
    task->fn = fn;
    task->next = nullptr;

    ThreadPoolTask *t = this->taskHead;
    if (t == nullptr) {
        this->taskHead = task;
    }
    else {
        while (t->next) {
            t = t->next;
        }
        t->next = task;
    }


    this->queueSize += 1;
    info(this->getInfo()) << "任务队列长度: " << this->queueSize;

    // 发出队列不空条件通知
    pthread_cond_signal(&this->queueCond);
    // 解锁线程池
    pthread_mutex_unlock(&this->mutexLock);
    return C_OK;
}


ThreadPoolTask *ThreadPool::dequeueTask() {
    // 锁住线程池
    pthread_mutex_lock(&this->mutexLock);
    // 如果任务队列空了，则阻塞等待队列不空
    while ((this->queueSize == 0) && !this->shutdown) {
        pthread_cond_wait(&this->queueCond, &this->mutexLock);
    }
    // 线程池关闭
    if (this->shutdown) {
        pthread_mutex_unlock(&this->mutexLock);
        return nullptr;
    }
    info(this->getInfo()) << "队列长度: " << this->queueSize;

    ThreadPoolTask *task = this->taskHead;
    this->taskHead = this->taskHead->next;
    this->queueSize -= 1;
    info(this->getInfo()) << "队列长度: " << this->queueSize;
    // 解锁线程池
    pthread_mutex_unlock(&this->mutexLock);
    return task;
}

void * ThreadPool::execute(void *threadPool) {
    auto *pool = (ThreadPool *)threadPool;

    while (true) {
        ThreadPoolTask *task;

        if ((task = pool->dequeueTask()) != nullptr) {
            // 执行任务
            info(pool->getInfo()) << "#" << pthread_self() << "执行" << task->name;
            (*(task->fn))(task->arg);

            delete task;
        }
    }

    return (void *) 0; // never reached
}


