//
// Created by liuliwu on 2020-05-29.
//

#include "connection.h"

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

Connection::~Connection() {

}