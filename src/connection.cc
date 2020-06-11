//
// Created by liuliwu on 2020-05-29.
//

#include "connection.h"
#include "log.h"
#include "db.h"
#include "http.h"
#include "command.h"
#include "threadpool_c.h"
#include "net_tcp.h"
#include <unistd.h>
#include <cerrno>

DEFINE_int32(max_connections, 100000, "最大的同时产生的客户端连接数");
DEFINE_bool(threads_connection, true, "是否使用线程处理客户端");

using namespace tLBS;

_Atomic uint64_t Connection::nextConnectionId = 0;
std::vector<Connection *> Connection::connections;

void Connection::setHttp(bool http) {
    this->http = http;
}

bool Connection::isHttp() {
    return this->http;
}

Connection::Connection(int fd, ConnectionState state) {
    this->id = ++Connection::nextConnectionId;
    this->fd = fd;
    this->state = state;
    this->flags = CONN_FLAGS_NONE;
    this->lastErrno = 0;
    this->refs = 0;
    char buf[100];
    snprintf(buf, sizeof(buf) - 1, "connection#%llu[fd:%d]", this->id, this->fd);
    this->info = buf;
    this->readHandler = nullptr;
    this->writeHandler = nullptr;
    this->http = false;
    this->db = Db::getDb(0);
//    this->connHandler = nullptr;
//    info("创建") << this->getInfo();
    connections.push_back(this);
}


uint64_t Connection::getId() {
    return this->id;
}

int Connection::getFd() {
    return this->fd;
}

void Connection::setFd(int fd) {
    this->fd = fd;
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

void Connection::setInfo(std::string info) {
    this->info = info;
}

std::string Connection::getInfo() {
    return this->info;
}

void Connection::pendingClose() {
    this->flags |= CONN_FLAGS_PENDING_CLOSE;
    this->container = nullptr;
//    info(this->getInfo()) << "conn pending close: " << this->flags;
}

bool Connection::isPendingClose() {
    return this->flags & CONN_FLAGS_PENDING_CLOSE;
}

// 只能通过关闭client来关闭connection
void Connection::close() {
    if (this->fd != -1) {
        if (!this->isHttp()) {
//            warning(this->getInfo()) << "连接关闭";
            EventLoop *el = EventLoop::getInstance();
            el->delFileEvent(this->fd, EL_READABLE);
            el->delFileEvent(this->fd, EL_WRITABLE);
        }
        if (::close(this->fd) < 0) {
            error(this->getInfo()) << "关闭出错: " << strerror(errno) << "(" << errno << ")";
        }
        else {
//            warning(this->getInfo()) << "关闭成功";
        }
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

int Connection::getLastErrno() {
    return this->lastErrno;
}

void Connection::setLastErrno(int lastErrno) {
    this->lastErrno = lastErrno;
}

Connection::~Connection() {
//    info("销毁") << this->getInfo();
}

ConnectionState Connection::getState() {
    return this->state;
}

void Connection::setState(ConnectionState state) {
    this->state = state;
}


void Connection::setContainer(void *container) {
    this->container = container;
}

void* Connection::getContainer() {
    return this->container;
}

int Connection::invokeHandler(ConnectionFallback handler) {
    if (handler) {
        handler(this);
    }
    return 1;
}

int Connection::setConnHandler(tLBS::ConnectionFallback handler) {
    EventLoop *el = EventLoop::getInstance();
    if (handler == this->getConnHandler()) {
        error(this->getInfo()) << "重复设置connection handler";
        return C_OK;
    }
    this->connHandler = handler;
    if (!this->connHandler) {
        el->delFileEvent(this->getFd(), EL_WRITABLE);
    }
    else {
        el->addFileEvent(this->getFd(), EL_WRITABLE, eventHandler, this);
    }
    return C_OK;
}


ConnectionFallback Connection::getConnHandler() {
    return this->connHandler;
}

int Connection::setWriteHandler(ConnectionFallback handler) {
    EventLoop *el = EventLoop::getInstance();
    if (handler == this->getWriteHandler()) {
        error(this->getInfo()) << "重复设置write handler";
        return C_OK;
    }
    this->writeHandler = handler;
    if (!this->writeHandler) {
        el->delFileEvent(this->getFd(), EL_WRITABLE);
    }
    else {
        el->addFileEvent(this->getFd(), EL_WRITABLE, eventHandler, this);
    }
    return C_OK;
}

ConnectionFallback Connection::getWriteHandler() {
    return this->writeHandler;
}

int Connection::setReadHandler(ConnectionFallback handler) {
    EventLoop *el = EventLoop::getInstance();
    if (handler == this->getReadHandler()) {
        error(this->getInfo()) << "重复设置read handler";
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


int Connection::getFlags() {
    return this->flags;
}

void Connection::setFlags(int flags) {
    this->flags = flags;
}

void Connection::eventHandler(int fd, int flags, void *data) {
    UNUSED(fd);
    auto *conn = (Connection *)data;

    if (conn->getState() == ConnectionState::CONN_STATE_CONNECTING && (flags & EL_WRITABLE) && (conn->getConnHandler() != nullptr)) {
        if (NetTcp::checkError(fd)) {
            conn->setLastErrno(errno);
            conn->setState(ConnectionState::CONN_STATE_ERROR);
        }
        else {
            // 设置为已连接
            conn->setState(ConnectionState::CONN_STATE_CONNECTED);
        }

        if (!conn->getWriteHandler()) {
            EventLoop *el = EventLoop::getInstance();
            el->delFileEvent(conn->getFd(), EL_WRITABLE);
        }

        if (!conn->invokeHandler(conn->getConnHandler())) {
            return;
        }
        conn->setConnHandler(nullptr);
    }

    int callRead = (flags & EL_READABLE) && conn->getReadHandler();
    int callWrite = (flags & EL_WRITABLE) && conn->getWriteHandler();

    if (callRead) {
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
}


int Connection::success() {
    return this->success(R"({"errno": 0, "data": "OK"})");

}

int Connection::success(tLBS::Json *json) {
    int ret = this->success(json->toString().c_str());
    delete json;
    return ret;
}

int Connection::success(const char *msg) {
    std::string str = msg;
    if (this->isHttp()) {
        std::string responseHeader = "HTTP/1.0 200 OK\r\nConnection: close\r\nContent-Type: application/json;charset=utf-8\r\nContent-Length: ";
        responseHeader += std::to_string(str.size());
        responseHeader += "\r\n\r\n";
        str = responseHeader + str;
    }
    else {
        str += "\r\n";
    }

    if (this->write(str.c_str(), str.size()) < 0) {
//        error(this->getInfo()) << strerror(errno) << "(" << errno << ")";
    }
    else {
//        info(this->getInfo()) << "写入成功";
    }
    if (this->isHttp()) {
        this->close();
    }
    else {
//        this->setReadHandler(connReadHandler);
    }
    return C_OK;
}

int Connection::fail(int error, const char *msg) {
    return this->fail(R"({"errno": %d, "data": "%s"})", error, msg);
}

int Connection::fail(tLBS::Json *json) {
    int ret = this->fail(json->toString().c_str());
    delete json;
    return ret;
}

int Connection::fail(const char *fmt, ...) {
    va_list ap;
    char msg[1024];
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    std::string str = msg;
//    info(this->getInfo()) << "失败" << pthread_self();
    if (this->isHttp()) {
        std::string responseHeader = "HTTP/1.0 200 OK\r\nConnection: close\r\nContent-Type: application/json;charset=utf-8\r\nContent-Length: ";
        responseHeader += std::to_string(str.size());
        responseHeader += "\r\n\r\n";
        str = responseHeader + str;
    }
    else {
        str += "\r\n";
    }
    this->write(str.c_str(), str.size());
    if (this->isHttp()) {
        this->close();
    }
    else {
//        this->setReadHandler(connReadHandler);
    }
    return C_ERR;
}


void Connection::setDb(tLBS::Db *db) {
    this->db = db;
}

Db* Connection::getDb() {
    return this->db;
}


Connection::ThreadArg::ThreadArg(tLBS::Connection *conn, std::string query) {
    this->connection = conn;
    this->query = query;
}

Connection* Connection::ThreadArg::getConnection() {
    return this->connection;
}

std::string Connection::ThreadArg::getQuery() {
    return this->query;
}

void * Connection::threadProcess(void *arg) {
    auto threadArg = (ThreadArg *)arg;
    Connection *conn = threadArg->getConnection();
//    pthread_detach(pthread_self());
    if (!conn->isHttp()) {
        Command::processCommandAndReset(conn, threadArg->getQuery());
    }
    else {
        Http::processHttpAndReset(conn, threadArg->getQuery());
    }
    return (void *)0;
}


void Connection::connReadHandler(Connection *conn) {
    if (conn->getState() == ConnectionState::CONN_STATE_CLOSED) {
        return;
    }
    int segLen = (1024); // 32M
    int totalRead = 0;
    std::string qb;
    char buf[segLen];
    while (true) {
        // 每次读出一部分
        memset(buf, 0, segLen);
        int nRead = conn->read(buf, segLen);
        if (nRead == -1) {
            if (conn->getLastErrno() > 0) {
//                info(conn->getInfo()) << "已读取(" << totalRead << "): " << qb;
                error(conn->getInfo()) << "读取数据错误: " << strerror(conn->getLastErrno()) << "(" << conn->getLastErrno() << ")";
                conn->pendingClose();
                return;
            }
            else {
//                error(conn->getInfo()) << "读取数据错误: " << strerror(conn->getLastErrno()) << "(" << conn->getLastErrno() << ")";
                break;
            }
        }
        else {
            if (nRead == 0) {
                if (strlen(buf) > 0) {
                    info(conn->getInfo()) << "被关闭？但是还是有数据: " << buf;
                    buf[segLen] = '\0'; // 确保最后一个字符是\0
                    // 有新的内容 追加进去
                    totalRead += strlen(buf);
                    qb += buf;
                    break;
                }
                else {
                    info(conn->getInfo()) << "客户端关闭连接,准备关闭";
                    conn->pendingClose();
                    return;
                }
            }
            else if (strlen(buf) > 0) {
                buf[segLen] = '\0'; // 确保最后一个字符是\0
                // 有新的内容 追加进去
                totalRead += strlen(buf);
                qb += std::string(buf);
            }
        }
    }
    if (totalRead == 0) {
        warning(conn->getInfo()) << "读取数据长度为0, " << conn->getInfo() << "准备关闭";
        conn->pendingClose();
        return;
    } else {
        info(conn->getInfo()) << "读取数据长度为: " << totalRead << std::endl
            << qb;
    }

    conn->setHttp(Http::connIsHttp(qb));

    if (FLAGS_threads_connection) {
        std::string taskName = "connection::threadProcess";
        taskName += conn->getInfo();
        // 使用线程处理
        ThreadPool::getPool("connection")
                ->enqueueTask(Connection::threadProcess, (void *)new ThreadArg(conn, qb), taskName);
        if (conn->isHttp()) {
            EventLoop *el = EventLoop::getInstance();
            el->delFileEvent(conn->getFd(), EL_READABLE);
            el->delFileEvent(conn->getFd(), EL_WRITABLE);
        }
    }
    else {
        if (!conn->isHttp()) {
            Command::processCommandAndReset(conn, qb);
        }
        else {
            Http::processHttpAndReset(conn, qb);
        }
    }
}


void Connection::adjustMaxConnections() {
    rlim_t maxFileNum = FLAGS_max_connections + MIN_REVERSED_FDS;
//    info("max fileno: ") << maxFileNum;
    struct rlimit limit;
    if (getrlimit(RLIMIT_NOFILE, &limit) == -1) {
        warning("无法获知当前系统的 NOFILE 的限制，假设是1024个来设置 max_connections");
        FLAGS_max_connections = 1024 - MIN_REVERSED_FDS;
    }
    else {
        rlim_t oldLimit = limit.rlim_cur;
//        info("current rlimit: ") << oldLimit;
        if (oldLimit < maxFileNum) {
            rlim_t bestLimit;
            int setResourceLimitErrno = 0;

            bestLimit = maxFileNum;
//            info("best rlimit: ") << bestLimit;
            while (bestLimit > oldLimit) {
                // 不断地尝试setrlimit 每次递减16
                limit.rlim_cur = bestLimit;
                limit.rlim_max = bestLimit;
                if (setrlimit(RLIMIT_NOFILE, &limit) != -1) {
                    break; // 设置成功了
                }
                // 记录设置错误
                setResourceLimitErrno = errno;
                if (bestLimit < 16) {
                    break; // 不能出现负值
                }
                bestLimit -= 16;
//                info("best rlimit: ") << bestLimit;
            }
            if (bestLimit < oldLimit) {
                bestLimit = oldLimit;
            }
//            info("best rlimit: ") << bestLimit;
            if (bestLimit < maxFileNum) {
                int oldMaxClients = FLAGS_max_connections;
                FLAGS_max_connections = bestLimit - MIN_REVERSED_FDS;
                if (bestLimit < MIN_REVERSED_FDS) {
                    fatal("系统当前的 'ulimit -n'为: ") << oldLimit
                                                  << "，不足以启动服务器，请修改系统的最大可打开的文件数量";
                    exit(1); // never reach
                }
                if (setResourceLimitErrno != 0) {
                    warning("服务无法设置可打开的最大文件数至: ") << maxFileNum
                                                  << ", 由于系统错误: " << strerror(setResourceLimitErrno);
                }
                info("原配置的max_connections为: ") << oldMaxClients
                                           << ", 现被修改为: " << FLAGS_max_connections;
            }
            else {
                info("原可打开的最大文件数为: ") << oldLimit
                                      << ", 被修改为: " << bestLimit;
            }
        }
    }
    info("适配最大可打开文件数的限制: ") << FLAGS_max_connections;
}



void Connection::destroyConnectionsIfNeed() {
    for (auto vecIter = connections.begin(); vecIter != connections.end();) {
        Connection *conn = *vecIter;
        if (conn->isPendingClose()) {
            vecIter = connections.erase(vecIter);
//            warning("销毁") << conn->getInfo();
            conn->close();
            delete conn;
        }
        else {
            vecIter++;
        }
    }
}

void Connection::destroyConnections() {
    for (auto vecIter = connections.begin(); vecIter != connections.end();) {
        Connection *conn = *vecIter;
        vecIter = connections.erase(vecIter);
//        warning("销毁") << conn->getInfo();
        conn->close();
        delete conn;
    }
}

int Connection::getConnectionsSize() {
    return connections.size();
}