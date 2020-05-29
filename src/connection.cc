//
// Created by liuliwu on 2020-05-29.
//

#include "connection.h"

#include <unistd.h>
#include <cerrno>

using namespace tLBS;

Connection::Connection() {
    this->fd = -1;
    this->state = ConnectionState::CONN_STATE_NONE;
    this->data = nullptr;
    this->flags = 0;
    this->lastErrno = 0;
    this->refs = 0;
}

Connection::Connection(int fd) {
    this->fd = fd;
    this->state = ConnectionState::CONN_STATE_ACCEPTING;
    this->data = nullptr;
    this->flags = 0;
    this->lastErrno = 0;
    this->refs = 0;
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

void Connection::close(EventLoop *el) {
    if (this->fd != -1) {
        el->delFileEvent(this->fd, EL_READABLE);
        el->delFileEvent(this->fd, EL_WRITABLE);
        ::close(this->fd);
        this->fd = -1;
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

Connection::~Connection() {

}