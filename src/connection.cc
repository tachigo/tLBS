//
// Created by liuliwu on 2020-05-29.
//

#include "connection.h"
#include "log.h"
#include "client.h"

#include <unistd.h>
#include <cerrno>

using namespace tLBS;


std::map<int, Connection *> Connection::connections;

Connection* Connection::getInstance(int fd) {
    auto mapIter = connections.find(fd);
    if (mapIter == connections.end()) {
        return connections[fd];
    }
    return nullptr;
}

Connection* Connection::create(int fd) {
    auto mapIter = connections.find(fd);
    if (mapIter == connections.end()) {
        connections[fd] = new Connection(fd);
    }
    return connections[fd];
//    return new Connection(fd);
}

Connection::Connection(int fd) {
    this->fd = fd;
    this->state = ConnectionState::CONN_STATE_ACCEPTING;
    this->data = nullptr;
    this->flags = 0;
    this->lastErrno = 0;
    this->refs = 0;
}

void Connection::setData(void *data) {
    this->data = data;
}

void* Connection::getData() {
    return this->data;
}

int Connection::getFd() {
    return this->fd;
}

void Connection::incrRefs() {
    this->refs++;
}

void Connection::decrRefs() {
    this->refs--;
}

int Connection::getRefs() {
    return this->refs;
}

std::string Connection::getInfo() {
    char buf[100];
    snprintf(buf, sizeof(buf) - 1, "fd=%i", this->fd);
    return buf;
}

void Connection::close() {
    if (this->fd != -1) {
        EventLoop *el = EventLoop::getInstance();
        el->delFileEvent(this->fd, EL_READABLE);
        el->delFileEvent(this->fd, EL_WRITABLE);
        ::close(this->fd);
        this->fd = -1;
        Client::free((Client *)this->data);
        free(this);
    }
}

int Connection::write(const void *data, size_t dataLen) {
    int ret = ::write(this->fd, data, dataLen);
    if (ret < 0 &&  errno != EAGAIN) {
        this->lastErrno = errno;
        this->state = ConnectionState::CONN_STATE_ERROR;
    }
    return ret;
}

int Connection::read(void *buf, size_t bufLen) {
    int ret = ::read(this->fd, buf, bufLen);
    if (!ret) {
        this->state = ConnectionState::CONN_STATE_CLOSED;
    } else if (ret < 0 && errno != EAGAIN) {
        this->lastErrno = errno;
        this->state = ConnectionState::CONN_STATE_ERROR;
    }
    return ret;
}

void Connection::free(tLBS::Connection *conn) {
    auto mapIter = connections.find(conn->getFd());
    if (mapIter != connections.end()) {
        connections.erase(conn->getFd());
    }
}

Connection::~Connection() {
    info("销毁connection#") << this->fd;
}

ConnectionState Connection::getState() {
    return this->state;
}

void Connection::setState(ConnectionState state) {
    this->state = state;
}

int Connection::invokeHandler(ConnectionFallback handler) {
    this->incrRefs();
    if (handler) {
        handler(this);
        sleep(100);
    }
    this->decrRefs();
    if (this->flags & CONN_FLAG_CLOSE_SCHEDULED) {
        if (this->getRefs()) {
            this->close();
        }
        else {
            return 0;
        }
    }
    return 1;
}

int Connection::setReadHandler(ConnectionFallback handler) {
    EventLoop *el = EventLoop::getInstance();
    if (handler == this->getReadHandler()) {
        return C_OK;
    }
    this->readHandler = handler;
    if (!this->readHandler) {
        el->delFileEvent(this->getFd(), EL_READABLE);
    }
    else {
        el->addFileEvent(this->getFd(), EL_READABLE, eventHandler, this);
    }
    return C_OK;
}

ConnectionFallback Connection::getReadHandler() {
    return this->readHandler;
}

int Connection::setWriteHandler(tLBS::ConnectionFallback handler) {

    return C_OK;
}

ConnectionFallback Connection::getWriteHandler() {
    return this->writeHandler;
}

int Connection::setConnectHandler(tLBS::ConnectionFallback handler) {

    return C_OK;
}

ConnectionFallback Connection::getConnectHandler() {
    return this->connectHandler;
}

int Connection::getFlags() {
    return this->flags;
}

void Connection::eventHandler(int fd, int flags, void *data) {
//    EventLoop *el = EventLoop::getInstance();
    UNUSED(fd);
    auto *conn = (Connection *)data;
//    if (conn->getState() == CONN_STATE_CONNECTING &&
//            ((flags & EL_WRITABLE) && conn->getWriteHandler())) {
//        conn->setState(CONN_STATE_CONNECTED);
//        if (!conn->getWriteHandler()) {
//            el->delFileEvent(conn->getFd(), EL_WRITABLE);
//        }
//        if (!conn->invokeHandler(conn->getConnectHandler())) {
//            return;
//        }
//        conn->setConnectHandler(nullptr);
//    }

    int invert = conn->getFlags() & CONN_FLAG_WRITE_BARRIER;
    int callRead = (flags & EL_READABLE) && conn->getReadHandler();
    int callWrite = (flags & EL_WRITABLE) && conn->getWriteHandler();

    if (!invert && callRead) {
        if (!conn->invokeHandler(conn->getReadHandler())) {
            warning("connection对象调用读句柄失败");
            return;
        }
    }
    if (callWrite) {
        if (!conn->invokeHandler(conn->getWriteHandler())) {
            warning("connect对象调用写句柄失败");
            return;
        }
    }
    if (invert && callRead) {
        if (!conn->invokeHandler(conn->getReadHandler())) {
            warning("connection对象调用读句柄失败");
            return;
        }
    }

}