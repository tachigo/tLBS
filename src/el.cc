//
// Created by liuliwu on 2020-05-28.
//

#include "el.h"
#include "log.h"

using namespace tLBS;


EventLoop::EventLoop(int setSize) {
    this->setSize = setSize;
    this->lastTime = time(nullptr);
    this->stop = false;
    this->maxFd = -1;
    this->flags = 0;
    // 创建io多路复用处理对象
    this->handler = new EventLoopHandler();
}

EventLoop::~EventLoop() {
    this->handler = nullptr;
}

int EventLoop::getSetSize() {
    return this->setSize;
}

EventLoopHandler *EventLoop::getHandler() {
    return this->handler;
}

void EventLoop::start() {
    this->stop = false;
    while (!this->stop) {
        this->processEvents();
    }
}


int EventLoop::processEvents() {
    info("处理事件");
}

void EventLoop::setStop() {
    this->stop = true;
}

bool EventLoop::isStop() {
    return this->stop;
}

std::string EventLoop::getName() {
    return this->handler->getName();
}