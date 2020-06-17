//
// Created by liuliwu on 2020-06-10.
//

#include "cluster.h"
#include "config.h"
#include "connection.h"
#include "log.h"
#include "net_tcp.h"
#include "threadpool_c.h"
#include "command.h"
#include "db.h"
#include "table.h"

#include <regex>
#include <sstream>
#include <fstream>


DEFINE_string(cluster_nodes, "", "cluster节点的链接字符串 eg. 127.0.0.1:8899;127.0.0.1:8888");

using namespace tLBS;

std::map<std::string, ClusterNode *> Cluster::nodes;

void Cluster::init() {
    if (FLAGS_cluster_nodes.size() > 0) {
        std::regex reg(";");
        std::vector<std::string> v(
                std::sregex_token_iterator(
                        FLAGS_cluster_nodes.begin(), FLAGS_cluster_nodes.end(), reg, -1
                ),
                std::sregex_token_iterator());
        for (int i = 0; i < v.size(); i++) {
            std::string clusterNodeUrl = v[i];
            addNode(clusterNodeUrl);
        }
    }
    else {
        warning("没有集群");
    }
    tryReady();
}


void Cluster::addNode(std::string nodeUrl, tLBS::Connection *conn) {
    std::regex reg(":");
    std::vector<std::string> v(
            std::sregex_token_iterator(
                    nodeUrl.begin(), nodeUrl.end(), reg, -1
            ),
            std::sregex_token_iterator());
    std::string ip = v[0];
    std::stringstream is(v[1]);
    int port;
    is >> port;
    auto node = new ClusterNode(ip, port);
    nodes[nodeUrl] = node;
    node->setConnection(conn);
    node->setFlags(node->getFlags() | CLUSTER_NODE_FLAGS_CONNECTED | CLUSTER_NODE_FLAGS_JOINED);
    node->setRole(node->getRole() | CLUSTER_NODE_ROLE_ACCEPTED);
    char buf[100];
    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf) - 1, " {cluster[%s:%d][fd:%d]} ", node->getIp().c_str(), node->getPort(), conn->getFd());
    node->setInfo(buf);
    conn->setInfo(node->getInfo());
    conn->setContainer(node);
    warning(conn->getInfo()) << "成功加入集群";
}

void Cluster::addNode(std::string nodeUrl) {
    std::regex reg(":");
    std::vector<std::string> v(
            std::sregex_token_iterator(
                    nodeUrl.begin(), nodeUrl.end(), reg, -1
            ),
            std::sregex_token_iterator());
    std::string ip = v[0];
    std::stringstream is(v[1]);
    int port;
    is >> port;
    auto node = new ClusterNode(ip, port);
    nodes[nodeUrl] = node;
}

void Cluster::connConnectHandler(tLBS::Connection *conn) {
    auto node = (ClusterNode *)conn->getContainer();
    if (conn->getState() != ConnectionState::CONN_STATE_CONNECTED) {
//        warning(conn->getInfo()) << " 建立连接失败: " << strerror(conn->getLastErrno()) << "(" << conn->getLastErrno() << ")";
        node->closeConnection();
        return;
    }

    conn->setInfo(node->getInfo());
    node->setConnection(conn);
    node->setFlags(node->getFlags() | CLUSTER_NODE_FLAGS_CONNECTED);
    node->setFlags(node->getFlags() &~ CLUSTER_NODE_FLAGS_CONNECTING);
    node->setRole(node->getRole() | CLUSTER_NODE_ROLE_CONNECT);
    // 设置读的handler
    conn->setReadHandler(connReadClusterJoinHandler);
    // 发送join
    joinCluster(conn);
}

void Cluster::tryReady() {
    // 尝试建立连接
    NetTcp *net = NetTcp::getInstance();
    for (auto mapIter = nodes.begin(); mapIter != nodes.end(); mapIter++) {
        auto node = mapIter->second;
        if (!(node->getFlags() & CLUSTER_NODE_FLAGS_CONNECTED)) {
            if (!(node->getFlags() & CLUSTER_NODE_FLAGS_CONNECTING)) {
                // 没有建立连接
                int cfd = net->connect(node->getIp().c_str(), node->getPort(), nullptr, NET_CONNECT_NONBLOCK | NET_CONNECT_BE_BINDING);
                if (cfd > 0) {
                    auto conn = new Connection(cfd, ConnectionState::CONN_STATE_CONNECTING);
                    node->setFlags(node->getFlags() | CLUSTER_NODE_FLAGS_CONNECTING);
                    char buf[100];
                    snprintf(buf, sizeof(buf) - 1, " {cluster[%s:%d][fd:%d]} ", node->getIp().c_str(), node->getPort(), conn->getFd());
                    node->setInfo(buf);
                    conn->setInfo(node->getInfo());
                    conn->setContainer((void *)node);
                    node->setConnection(conn);
                    conn->setConnHandler(connConnectHandler);
                }
            }
        }
        else if (!(node->getFlags() & CLUSTER_NODE_FLAGS_JOINED)) {
            if (!(node->getFlags() & CLUSTER_NODE_FLAGS_JOINING)) {
                auto conn = node->getConnection();
                joinCluster(conn);
            }
        }
        else if (node->getRole() == CLUSTER_NODE_ROLE_ACCEPTED &&
                !(node->getFlags() & CLUSTER_NODE_FLAGS_RECEIVER_SYNC_OK) && !(node->getFlags() & CLUSTER_NODE_FLAGS_RECEIVER_SYNC_ING)) {
            auto conn = node->getConnection();
            startSyncCluster(conn);
            return;
        }
        else if (node->getFlags() & CLUSTER_NODE_FLAGS_ESTABLISH) {
            auto conn = node->getConnection();
            pingCluster(conn);
        }
        else {
            if ((node->getFlags() & CLUSTER_NODE_FLAGS_SENDER_SYNC_OK) && (node->getFlags() & CLUSTER_NODE_FLAGS_RECEIVER_SYNC_OK)) {
                warning(node->getInfo()) << "连接完成 Established!";
                node->setFlags(node->getFlags() | CLUSTER_NODE_FLAGS_ESTABLISH);
            }
        }
    }
}

int Cluster::connRead(Connection *conn, std::string *qb) {
    if (conn->getState() == ConnectionState::CONN_STATE_CLOSED) {
        return C_ERR;
    }
    auto node = (ClusterNode *)conn->getContainer();
    int segLen = (1024); // 32M
    int totalRead = 0;
    char buf[segLen];
    while (true) {
        // 每次读出一部分
        memset(buf, 0, segLen);
        int nRead = conn->read(buf, segLen);
        if (nRead == -1) {
            if (conn->getLastErrno() > 0) {
                error(conn->getInfo()) << "读取数据错误,节点准备断开: " << strerror(conn->getLastErrno()) << "(" << conn->getLastErrno() << ")";
                node->closeConnection();
                return C_ERR;
            }
            else {
                break;
            }
        }
        else {
            if (nRead == 0) {
                if (strlen(buf) > 0) {
                    buf[segLen] = '\0'; // 确保最后一个字符是\0
                    // 有新的内容 追加进去
                    totalRead += strlen(buf);
                    *qb += buf;
                    break;
                }
                else {
                    info(conn->getInfo()) << "客户端关闭连接,节点准备断开";
                    node->closeConnection();
                    return C_ERR;
                }
            }
            else if (strlen(buf) > 0) {
                buf[segLen] = '\0'; // 确保最后一个字符是\0
                // 有新的内容 追加进去
                totalRead += strlen(buf);
                *qb += std::string(buf);
            }
        }
    }
    if (totalRead == 0) {
        warning(conn->getInfo()) << "读取数据长度为0, " << conn->getInfo() << "准备断开";
        node->closeConnection();
        return C_ERR;
    }

    return C_OK;
}

// join
// send
int Cluster::joinCluster(tLBS::Connection *conn) {
    if (conn->getReadHandler() != connReadClusterJoinHandler) {
        conn->setReadHandler(connReadClusterJoinHandler);
    }
    auto node = (ClusterNode *)conn->getContainer();
    node->setFlags(node->getFlags() | CLUSTER_NODE_FLAGS_JOINING);
    warning(CUR_SERVER) << "向" << conn->getInfo() << "请求加入集群";
    char msg[1024];
    memset(msg, 0, sizeof(msg));
    snprintf(msg, sizeof(msg) - 1, "clusterjoin %s:%s", FLAGS_tcp_host.c_str(), FLAGS_tcp_port.c_str());
    return conn->success(msg);
}
// recv
int Cluster::execClusterJoin(tLBS::Connection *conn, std::vector<std::string> args) {
    if (args.size() != 2) {
        return conn->fail(Json::createErrorJsonObj(ERRNO_EXEC_SYNTAX_ERR, ERROR_EXEC_SYNTAX_ERR));
    }
    warning(CUR_SERVER) << "从" << conn->getInfo() << "收到加入集群请求";
    std::string addr = args[1];
    addNode(addr, conn);
    return conn->success(Json::createSuccessStringJsonObj("OK"));
}
// reader
void Cluster::connReadClusterJoinHandler(tLBS::Connection *conn) {
    conn->setReadHandler(nullptr);
    auto node = (ClusterNode *)conn->getContainer();
    std::string qb = "";
    if (connRead(conn, &qb) != C_OK) {
        return;
    }
    auto json = new Json(qb);
    if (json->get("data") == "OK") {
        warning(conn->getInfo()) << "成功加入集群";
        node->setFlags(node->getFlags() | CLUSTER_NODE_FLAGS_JOINED);
    }
    else {
        warning(conn->getInfo()) << "加入集群失败";
        node->setFlags(node->getFlags() &~ CLUSTER_NODE_FLAGS_JOINED);
    }
    node->setFlags(node->getFlags() &~ CLUSTER_NODE_FLAGS_JOINING);
    startSyncCluster(conn);
}

// sync start
// send
int Cluster::startSyncCluster(tLBS::Connection *conn) {
    if (conn->getReadHandler() != connReadClusterStartSyncHandler) {
        conn->setReadHandler(connReadClusterStartSyncHandler);
    }
    auto node = (ClusterNode *)conn->getContainer();
    if (node->getRole() == CLUSTER_NODE_ROLE_CONNECT) {
        node->setFlags(node->getFlags() | CLUSTER_NODE_FLAGS_SENDER_SYNC_ING);
    }
    if (node->getRole() == CLUSTER_NODE_ROLE_ACCEPTED) {
        node->setFlags(node->getFlags() | CLUSTER_NODE_FLAGS_RECEIVER_SYNC_ING);
    }
    warning(CUR_SERVER) << "向" << conn->getInfo() << "请求准备数据同步";
    std::string msg = "clusterstartsync";
    return conn->success(msg.c_str());
}
// recv
int Cluster::execClusterStartSync(tLBS::Connection *conn, std::vector<std::string> args) {
    warning(CUR_SERVER) << "从" << conn->getInfo() << "收到准备数据同步请求";
    UNUSED(args);
    auto node = (ClusterNode *)conn->getContainer();
    if (node->getRole() == CLUSTER_NODE_ROLE_CONNECT) {
        node->setFlags(node->getFlags() | CLUSTER_NODE_FLAGS_RECEIVER_SYNC_ING);
    }
    if (node->getRole() == CLUSTER_NODE_ROLE_ACCEPTED) {
        node->setFlags(node->getFlags() | CLUSTER_NODE_FLAGS_SENDER_SYNC_ING);
    }
    Json *response = Json::createSuccessArrayJsonObj();
    Json *list = new Json(R"([])");
    std::vector<Db *> dbs = Db::getDbs();
    for (int i = 0; i < dbs.size(); i++) {
        Db *db = dbs[i];
        // 从文件中读取
        std::string datFile = FLAGS_db_root + db->getDatFile();
        std::ifstream ifs(datFile);
        if (ifs) {
            std::string line;
            while (std::getline(ifs, line)) {
                list->value().PushBack(Json::createString(line), list->getAllocator());
            }
            ifs.close();
        }
    }
    response->get("data") = list->value();
    return conn->success(response);
}
// reader
void Cluster::connReadClusterStartSyncHandler(tLBS::Connection *conn) {
    conn->setReadHandler(nullptr);
    auto node = (ClusterNode *)conn->getContainer();
    std::string qb = "";
    if (connRead(conn, &qb) != C_OK) {
        return;
    }
    auto json = new Json(qb);
    if (json->get("errno") != 0) {
        warning(CUR_SERVER) << "从" << conn->getInfo() << "获取集群数据同步信息失败";
        if (node->getRole() == CLUSTER_NODE_ROLE_CONNECT) {
            node->setFlags(node->getFlags() &~ CLUSTER_NODE_FLAGS_SENDER_SYNC_ING);
        }
        if (node->getRole() == CLUSTER_NODE_ROLE_ACCEPTED) {
            node->setFlags(node->getFlags() &~ CLUSTER_NODE_FLAGS_RECEIVER_SYNC_ING);
        }
    }
    else {
        warning(CUR_SERVER) << "从" << conn->getInfo() << "获取集群数据同步信息成功";
        // 要发送的数据
        std::string tables = "";
        // 遍历数据
        const rapidjson::Value& arr = json->get("data");
        for (auto iter = arr.Begin(); iter != arr.End(); iter++) {
            std::vector<std::string> v = splitString(iter->GetString(), "/");
            int dbNo = atoi(v[0].c_str());
            std::string tableName = v[1];
            if (dbNo >= FLAGS_db_num) {
                error(conn->getInfo()) << "不支持的db";
                continue;
            }
            Db *db = Db::getDb(dbNo);
            Table *tableObj = db->lookupTableWrite(tableName);
            if (tableObj == nullptr) {
                // 如果table不存在
                tableObj = Table::parseMetadata(iter->GetString());
                tableObj->setVersion(0); // 重置版本号
                db->tableAdd(tableName, tableObj);
            }
            // 根据版本号判断是否需要同步数据
            int version = atoi(v[5].c_str());
            if (version > tableObj->getVersion()) {
                // 需要同步数据
                tables += "," + v[0] + ":" + tableName;
            }
        }
        if (tables.size() > 0) {
            // 发送需要同步的表给另一端
            warning(CUR_SERVER) << "从" << conn->getInfo() << "中 有 需要进行同步的数据";
            doSyncCluster(conn, tables.substr(1, tables.size()));
        }
        else {
            warning(CUR_SERVER) << "从" << conn->getInfo() << "中 无 需要进行同步的数据";
            endSyncCluster(conn);
        }
    }
}


// do sync
// send
int Cluster::doSyncCluster(tLBS::Connection *conn, std::string data) {
    if (conn->getReadHandler() != connReadClusterDoSyncHandler) {
        conn->setReadHandler(connReadClusterDoSyncHandler);
    }
    auto node = (ClusterNode *)conn->getContainer();
    if (node->getRole() == CLUSTER_NODE_ROLE_CONNECT) {
        node->setFlags(node->getFlags() | CLUSTER_NODE_FLAGS_SENDER_SYNC_ING);
    }
    if (node->getRole() == CLUSTER_NODE_ROLE_ACCEPTED) {
        node->setFlags(node->getFlags() | CLUSTER_NODE_FLAGS_RECEIVER_SYNC_ING);
    }
    warning(CUR_SERVER) << "向" << conn->getInfo() << "请求开始数据同步";
    std::string msg = "clusterdosync " + data;
    return conn->success(msg.c_str());
}
// recv
int Cluster::execClusterDoSync(tLBS::Connection *conn, std::vector<std::string> args) {
    warning(CUR_SERVER) << "从" << conn->getInfo() << "收到开始数据同步请求";
    auto node = (ClusterNode *)conn->getContainer();
    if (node->getRole() == CLUSTER_NODE_ROLE_CONNECT) {
        node->setFlags(node->getFlags() | CLUSTER_NODE_FLAGS_RECEIVER_SYNC_ING);
    }
    if (node->getRole() == CLUSTER_NODE_ROLE_ACCEPTED) {
        node->setFlags(node->getFlags() | CLUSTER_NODE_FLAGS_SENDER_SYNC_ING);
    }
    std::vector<std::string> tables = splitString(args[1], ",");
    for (int i = 0; i < tables.size(); i++) {
        std::vector<std::string> t = splitString(tables[i], ":");
        int dbNo = atoi(t[0].c_str());
        std::string tableName = t[1];
        Db *db = Db::getDb(dbNo);
        Table *tableObj = db->lookupTableRead(tableName);
        // 不断的降数据发送过去
        char identify[1024];
        memset(identify, 0, sizeof(identify));
        snprintf(identify, sizeof(identify), "%0.2d/%s/%d", dbNo, tableName.c_str(), tableObj->getVersion());
        tableObj->callSenderHandler(db->getDataPath(), std::string(identify), conn);
    }
    // 最后发送同步结束命令
    char msg[1024];
    memset(msg, 0, sizeof(msg));
    snprintf(msg, sizeof(msg), "clusterfinishsync");
    return conn->write(msg, strlen(msg));
}
// reader
void Cluster::connReadClusterDoSyncHandler(tLBS::Connection *conn) {
    std::string qb = "";
    if (connRead(conn, &qb) != C_OK) {
        return;
    }
    warning(CUR_SERVER) << "从" << conn->getInfo() << "同步数据进行中...";

    std::vector<std::string> lines = splitString(qb, "\n");
    for (int i = 0; i < lines.size(); i++) {
        if (lines[i] == "clusterfinishsync") {
            conn->setReadHandler(nullptr);
            endSyncCluster(conn);
            return;
        }
        std::vector<std::string> arr = splitString(lines[i], " ");
        std::vector<std::string> tableArr = splitString(arr[0], "/");
        int dbNo = atoi(tableArr[0].c_str());
        std::string tableName = tableArr[1];
        int version = atoi(tableArr[2].c_str());
        Db *db = Db::getDb(dbNo);
        Table *table = db->lookupTableWrite(tableName);
        if (table->getVersion() < version) {
            table->setVersion(version - 1); // 减小一个版本 后边保存的时候就会加上
        }
        std::string data = arr[1];
        table->callReceiverHandler(data);
        table->incrDirty(1);
    }
}


// end sync
// send
int Cluster::endSyncCluster(tLBS::Connection *conn) {
    if (conn->getReadHandler() != connReadClusterEndSyncHandler) {
        conn->setReadHandler(connReadClusterEndSyncHandler);
    }
    auto node = (ClusterNode *)conn->getContainer();
    if (node->getRole() == CLUSTER_NODE_ROLE_CONNECT) {
        node->setFlags(node->getFlags() | CLUSTER_NODE_FLAGS_SENDER_SYNC_ING);
    }
    if (node->getRole() == CLUSTER_NODE_ROLE_ACCEPTED) {
        node->setFlags(node->getFlags() | CLUSTER_NODE_FLAGS_RECEIVER_SYNC_ING);
    }
    warning(CUR_SERVER) << "向" << conn->getInfo() << "请求数据同步结束";
    std::string msg = "clusterendsync";
    return conn->success(msg.c_str());
}
// recv
int Cluster::execClusterEndSync(tLBS::Connection *conn, std::vector<std::string> args) {
    warning(CUR_SERVER) << "从" << conn->getInfo() << "收到数据同步结束请求";
    UNUSED(args);
    auto node = (ClusterNode *)conn->getContainer();
    if (node->getRole() == CLUSTER_NODE_ROLE_CONNECT) {
        node->setFlags(node->getFlags() | CLUSTER_NODE_FLAGS_RECEIVER_SYNC_OK);
        node->setFlags(node->getFlags() &~ CLUSTER_NODE_FLAGS_RECEIVER_SYNC_ING);
    }
    if (node->getRole() == CLUSTER_NODE_ROLE_ACCEPTED) {
        node->setFlags(node->getFlags() | CLUSTER_NODE_FLAGS_SENDER_SYNC_OK);
        node->setFlags(node->getFlags() &~ CLUSTER_NODE_FLAGS_SENDER_SYNC_ING);
    }
    std::string msg = "clusterendsyncack";
    conn->write(msg.c_str(), msg.size());
    return C_OK;
}
// reader
void Cluster::connReadClusterEndSyncHandler(tLBS::Connection *conn) {
    std::string qb = "";
    if (connRead(conn, &qb) != C_OK) {
        return;
    }
    conn->setReadHandler(nullptr);
    if (qb == "clusterendsyncack") {
        warning(CUR_SERVER) << "从" << conn->getInfo() << "收到数据同步结束确认";
        auto node = (ClusterNode *)conn->getContainer();
        if (node->getRole() == CLUSTER_NODE_ROLE_CONNECT) {
            node->setFlags(node->getFlags() | CLUSTER_NODE_FLAGS_SENDER_SYNC_OK);
            node->setFlags(node->getFlags() &~ CLUSTER_NODE_FLAGS_SENDER_SYNC_ING);
            warning(CUR_SERVER) << "的" << conn->getInfo() << "是 SENDER" << std::endl
                << "SENDER_SYNC_OK: " << ((node->getFlags() & CLUSTER_NODE_FLAGS_SENDER_SYNC_OK) ? "是" : "否") << std::endl
                << "RECEIVER_SYNC_OK: " << ((node->getFlags() & CLUSTER_NODE_FLAGS_RECEIVER_SYNC_OK) ? "是" : "否");
        }
        if (node->getRole() == CLUSTER_NODE_ROLE_ACCEPTED) {
            node->setFlags(node->getFlags() | CLUSTER_NODE_FLAGS_RECEIVER_SYNC_OK);
            node->setFlags(node->getFlags() &~ CLUSTER_NODE_FLAGS_RECEIVER_SYNC_ING);
            warning(CUR_SERVER) << "的" << conn->getInfo() << "是 RECEIVER" << std::endl
                << "SENDER_SYNC_OK: " << ((node->getFlags() & CLUSTER_NODE_FLAGS_SENDER_SYNC_OK) ? "是" : "否") << std::endl
                << "RECEIVER_SYNC_OK: " << ((node->getFlags() & CLUSTER_NODE_FLAGS_RECEIVER_SYNC_OK) ? "是" : "否");
        }
    }
    if (conn->getReadHandler() != connReadHandler) {
        conn->setReadHandler(connReadHandler);
    }
}

int Cluster::execClusterNodes(Connection *conn, std::vector<std::string> args) {
    UNUSED(args);
    Json *dataList = new Json(R"({"total": 0, "list": []})");
    dataList->get("total").SetInt(nodes.size());
    for (auto mapIter = nodes.begin(); mapIter != nodes.end(); mapIter++) {
        Json *dataItem = new Json(R"({"addr": "", "flags": 0})");
        auto node = mapIter->second;
        dataItem->get("addr").SetString(Json::createString(mapIter->first));
        dataItem->get("flags").SetInt64(node->getFlags());
        dataList->get("list").PushBack(dataItem->value(), dataList->getAllocator());
    }
    Json *response = Json::createSuccessObjectJsonObj();
    response->get("data") = dataList->value();
    return conn->success(response);
}

int Cluster::pingCluster(Connection *conn) {
    if (conn->getReadHandler() != connReadHandler) {
        conn->setReadHandler(connReadHandler);
    }
    char msg[1024];
    memset(msg, 0, sizeof(msg));
    snprintf(msg, sizeof(msg) - 1, "ping %s:%s", FLAGS_tcp_host.c_str(), FLAGS_tcp_port.c_str());
    info(CUR_SERVER) << "向" << conn->getInfo() << "发送 " << msg << " ... ============>";
    return conn->success(msg);
}

void Cluster::broadcast(std::string cmd) {
    for (auto mapIter = nodes.begin(); mapIter != nodes.end(); mapIter++) {
        auto node = mapIter->second;
        if (node->getFlags() & CLUSTER_NODE_FLAGS_ESTABLISH) {
            // 已经建立连接且加入集群的
            auto conn = node->getConnection();
            conn->success(cmd.c_str());
        }
    }
}


void Cluster::free() {
    info("销毁所有cluster节点");
    std::vector<ClusterNode *> nv;
    for (auto mapIter = nodes.begin(); mapIter != nodes.end(); mapIter++) {
        nv.push_back(mapIter->second);
    }
    for (int i = 0; i < nv.size(); i++) {
        auto node = nv[i];
        nodes.erase(node->getInfo());
        delete node;
    }
}

Cluster::ThreadArg::ThreadArg(tLBS::Connection *conn, std::string query) {
    this->connection = conn;
    this->query = query;
}

Connection* Cluster::ThreadArg::getConnection() {
    return this->connection;
}

std::string Cluster::ThreadArg::getQuery() {
    return this->query;
}

void * Cluster::threadProcess(void *arg) {
    auto threadArg = (ThreadArg *)arg;
    Connection *conn = threadArg->getConnection();
    Command::processCommandAndReset(conn, threadArg->getQuery(), true);
    return (void *)0;
}



// 读取连接的其他节点发送过来的数据
void Cluster::connReadHandler(tLBS::Connection *conn) {
    std::string qb = "";
    if (connRead(conn, &qb) != C_OK) {
        return;
    }
//    info(conn->getInfo()) << "读取数据长度为: " << qb.size() << "\t===================>" << std::endl
//            << qb;

    if (FLAGS_threads_connection > 0) {
        std::string taskName = "cluster::threadProcess";
        taskName += conn->getInfo();
        // 使用线程处理
        ThreadPool::getPool("connection")
                ->enqueueTask(Cluster::threadProcess, (void *)new ThreadArg(conn, qb), taskName);
    }
    else {
        Command::processCommandAndReset(conn, qb, true);
    }
}

void Cluster::connWriteHandler(tLBS::Connection *conn) {
//    conn->setWriteHandler(nullptr);
}


ClusterNode::ClusterNode(std::string ip, int port) {
    this->ip = ip;
    this->port = port;
    this->conn = nullptr;
    this->flags = CLUSTER_NODE_FLAGS_NONE;
    this->role = CLUSTER_NODE_ROLE_NONE;

    char buf[100];
    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf) - 1, " {cluster[%s:%d][fd:未确定]} ", ip.c_str(), port);
    this->info = buf;
}

ClusterNode::~ClusterNode() {
    closeConnection();
}

void ClusterNode::setConnection(tLBS::Connection *conn) {
    this->conn = conn;
}

Connection* ClusterNode::getConnection() {
    return this->conn;
}

std::string ClusterNode::getInfo() {
    return this->info;
}

void ClusterNode::setInfo(std::string info) {
    this->info = info;
}

std::string ClusterNode::getIp() {
    return this->ip;
}

int ClusterNode::getPort() {
    return this->port;
}

void ClusterNode::setFlags(uint64_t flags) {
    this->flags = flags;
}

uint64_t ClusterNode::getFlags() {
    return this->flags;
}


void ClusterNode::setRole(int role) {
    this->role = role;
}

int ClusterNode::getRole() {
    return this->role;
}

void ClusterNode::closeConnection() {
    if (this->conn != nullptr) {
        this->conn->pendingClose();
        this->conn = nullptr;
        this->flags = CLUSTER_NODE_FLAGS_NONE;
        this->role = CLUSTER_NODE_ROLE_NONE;

        char buf[100];
        memset(buf, 0, sizeof(buf));
        snprintf(buf, sizeof(buf) - 1, " {cluster[%s:%d][fd:未确定]} ", ip.c_str(), port);
        this->info = buf;
    }
}
