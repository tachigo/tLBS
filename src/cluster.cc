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

int Cluster::unEstablishedNodeCount = 0;

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
        unEstablishedNodeCount = v.size();
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
    auto clusterNode = new ClusterNode(ip, port);
    nodes[nodeUrl] = clusterNode;
    clusterNode->setConnection(conn);
    clusterNode->setEstablished(true);
    clusterNode->setJoined(true);
    char buf[100];
    snprintf(buf, sizeof(buf) - 1, "cluster[%s:%d][fd:%d]", clusterNode->getIp().c_str(), clusterNode->getPort(), conn->getFd());
    clusterNode->setInfo(buf);
    conn->setInfo(clusterNode->getInfo());
    conn->setContainer(clusterNode);
    warning("cluster添加节点: ") << clusterNode->getInfo();
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
    auto clusterNode = new ClusterNode(ip, port);
    nodes[nodeUrl] = clusterNode;
    info("配置cluster节点: ") << clusterNode->getInfo() << "准备加入...";
}

void Cluster::connConnectHandler(tLBS::Connection *conn) {
    auto node = (ClusterNode *)conn->getContainer();
    if (conn->getState() != ConnectionState::CONN_STATE_CONNECTED) {
//        warning(conn->getInfo()) << " 建立连接失败: " << strerror(conn->getLastErrno()) << "(" << conn->getLastErrno() << ")";
        node->closeConnection();
        decrUnEstablishedNodeCount();
        return;
    }

    warning(node->getInfo()) << " 建立连接成功";
    decrUnEstablishedNodeCount();
    conn->setInfo(node->getInfo());
    node->setConnection(conn);
    node->setEstablished(true);
    // 设置读的handler
    conn->setReadHandler(connReadClusterJoinHandler);
    // 发送join
    joinCluster(conn);
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
//                info(conn->getInfo()) << "已读取(" << totalRead << "): " << qb;
                error(conn->getInfo()) << "读取数据错误: " << strerror(conn->getLastErrno()) << "(" << conn->getLastErrno() << ")";
                node->closeConnection();
                return C_ERR;
            }
            else {
//                error(conn->getInfo()) << "读取数据错误: " << strerror(conn->getLastErrno()) << "(" << conn->getLastErrno() << ")";
                break;
            }
        }
        else {
            if (nRead == 0) {
                if (strlen(buf) > 0) {
//                    info(conn->getInfo()) << "被关闭？但是还是有数据: " << buf;
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

void Cluster::connReadClusterJoinHandler(tLBS::Connection *conn) {
    conn->setReadHandler(nullptr);
    auto node = (ClusterNode *)conn->getContainer();
    std::string qb = "";
    if (connRead(conn, &qb) != C_OK) {
        return;
    }
//    info(conn->getInfo()) << "读取数据长度为: " << qb.size() << "\t===================>" << std::endl
//        << qb;

    auto json = new Json(qb);
    if (json->get("data") == "OK") {
        warning(conn->getInfo()) << "成功加入集群";
        node->setJoined(true);
    }
    else {
        warning(conn->getInfo()) << "加入集群失败";
        node->setJoined(false);
    }
}

void Cluster::connReadClusterSyncHandler(tLBS::Connection *conn) {
    conn->setReadHandler(nullptr);
    auto node = (ClusterNode *)conn->getContainer();
    std::string qb = "";
    if (connRead(conn, &qb) != C_OK) {
        return;
    }
//    info(conn->getInfo()) << "读取数据长度为: " << qb.size() << "\t===================>" << std::endl
//        << qb;

    auto json = new Json(qb);
    if (json->get("errno") != 0) {
        warning(conn->getInfo()) << "获取集群数据同步信息失败";
        node->setSynchronizing(false);
    }
    else {
        warning(conn->getInfo()) << "获取集群数据同步信息成功";
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
                info(conn->getInfo()) << "需要同步: " << iter->GetString();
            }
        }
    }

    // 设置为非正在同步
//    node->setSynchronizing(false);
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
//    pthread_detach(pthread_self());
    Command::processCommandAndReset(conn, threadArg->getQuery(), true);
    return (void *)0;
}



// 读取连接的其他节点发送过来的数据
void Cluster::connReadHandler(tLBS::Connection *conn) {
    std::string qb = "";
    if (connRead(conn, &qb) != C_OK) {
        return;
    }
    info(conn->getInfo()) << "读取数据长度为: " << qb.size() << "\t===================>" << std::endl
        << qb;
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

void Cluster::tryReady() {
    // 尝试建立连接
    NetTcp *net = NetTcp::getInstance();
    for (auto mapIter = nodes.begin(); mapIter != nodes.end(); mapIter++) {
        auto node = mapIter->second;
        if (!node->getEstablished()) {
            // 没有建立连接
            int cfd = net->connect(node->getIp().c_str(), node->getPort(), nullptr, NET_CONNECT_NONBLOCK | NET_CONNECT_BE_BINDING);
            if (cfd > 0) {
                auto conn = new Connection(cfd, ConnectionState::CONN_STATE_CONNECTING);

                char buf[100];
                snprintf(buf, sizeof(buf) - 1, "cluster[%s:%d][fd:%d]", node->getIp().c_str(), node->getPort(), conn->getFd());
                node->setInfo(buf);

                conn->setInfo(node->getInfo());
                conn->setContainer((void *)node);
                node->setConnection(conn);
                conn->setConnHandler(connConnectHandler);
            }
        }
        else if (!node->getJoined()) {
            // 没有加入集群
            auto conn = node->getConnection();
            if (conn->getReadHandler() != connReadClusterJoinHandler) {
                conn->setReadHandler(connReadClusterJoinHandler);
            }
            joinCluster(conn);
        }
        else if (!node->getSynced()) {
            if (!node->getSynchronizing()) {
                // 设置为正在进行同步
                node->setSynchronizing(true);
                // 如果没有尝试同步过数据
                auto conn = node->getConnection();
                if (conn->getReadHandler() != connReadClusterSyncHandler) {
                    conn->setReadHandler(connReadClusterSyncHandler);
                }
                syncCluster(conn);
            }
        }
        else{
            // 进行ping
            auto conn = node->getConnection();
            if (conn->getReadHandler() != Cluster::connReadHandler) {
                conn->setReadHandler(Cluster::connReadHandler);
            }
            pingCluster(conn);
        }
    }
}


void Cluster::broadcast(std::string cmd) {
    for (auto mapIter = nodes.begin(); mapIter != nodes.end(); mapIter++) {
        auto node = mapIter->second;
        if (node->getEstablished() && node->getJoined()) {
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


void Cluster::incrUnEstablishedNodeCount() {
    unEstablishedNodeCount += 1;
//    info("当前未建立cluster连接的节点有: ") << unEstablishedNodeCount;
}

void Cluster::decrUnEstablishedNodeCount() {
    unEstablishedNodeCount -= 1;
//    info("当前未建立cluster连接的节点有: ") << unEstablishedNodeCount;
}



ClusterNode::ClusterNode(std::string ip, int port) {
    this->ip = ip;
    this->port = port;
    this->conn = nullptr;
    this->established = false;
    this->joined = false;
    this->synced = false;
    this->synchronizing = false;
    char buf[100];
    snprintf(buf, sizeof(buf) - 1, "cluster[%s:%d]", ip.c_str(), port);
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

void ClusterNode::setEstablished(bool established) {
    this->established = established;
}

bool ClusterNode::getEstablished() {
    return this->established;
}

void ClusterNode::setJoined(bool joined) {
    this->joined = joined;
}

bool ClusterNode::getJoined() {
    return this->joined;
}


void ClusterNode::setSynced(bool synced) {
    this->synced = synced;
}

bool ClusterNode::getSynced() {
    return this->synced;
}

void ClusterNode::setSynchronizing(bool synchronizing) {
    this->synchronizing = synchronizing;
}

bool ClusterNode::getSynchronizing() {
    return this->synchronizing;
}


void ClusterNode::closeConnection() {
    if (this->conn != nullptr) {
        this->conn->pendingClose();
        this->conn = nullptr;
        Cluster::incrUnEstablishedNodeCount();
    }
    this->established = false;
    this->joined = false;
    this->synced = false;
    this->synchronizing = false;
}

// send
int Cluster::joinCluster(tLBS::Connection *conn) {
    warning(conn->getInfo()) << "进行加入集群";
    char msg[1024];
    snprintf(msg, sizeof(msg) - 1, "clusterjoin %s:%s", FLAGS_tcp_host.c_str(), FLAGS_tcp_port.c_str());
    return conn->success(msg);
}

// recv
int Cluster::execClusterJoin(tLBS::Connection *conn, std::vector<std::string> args) {
    if (args.size() != 2) {
        return conn->fail(Json::createErrorJsonObj(ERRNO_EXEC_SYNTAX_ERR, ERROR_EXEC_SYNTAX_ERR));
    }
    std::string addr = args[1];
    addNode(addr, conn);
    return conn->success(Json::createSuccessStringJsonObj("OK"));
}

int Cluster::pingCluster(Connection *conn) {
//    info(conn->getInfo()) << " ping...";
    char msg[1024];
    snprintf(msg, sizeof(msg) - 1, "ping %s:%s", FLAGS_tcp_host.c_str(), FLAGS_tcp_port.c_str());
    return conn->success(msg);
}


// send
int Cluster::syncCluster(tLBS::Connection *conn) {
    char msg[1024];
    snprintf(msg, sizeof(msg) - 1, "clustersync");
    return conn->success(msg);
}


// recv
int Cluster::execClusterSync(tLBS::Connection *conn, std::vector<std::string> args) {
    UNUSED(args);
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



int Cluster::execClusterNodes(Connection *conn, std::vector<std::string> args) {
    UNUSED(args);
    Json *dataList = new Json(R"({"total": 0, "list": []})");
    dataList->get("total").SetInt(nodes.size());
    for (auto mapIter = nodes.begin(); mapIter != nodes.end(); mapIter++) {
        Json *dataItem = new Json(R"({"addr": "", "established": false, "joined": false, "synced": false})");
        auto node = mapIter->second;
        dataItem->get("addr").SetString(Json::createString(mapIter->first));
        dataItem->get("established").SetBool(node->getEstablished());
        dataItem->get("joined").SetBool(node->getJoined());
        dataItem->get("synced").SetBool(node->getSynced());
        dataList->get("list").PushBack(dataItem->value(), dataList->getAllocator());
    }
    Json *response = Json::createSuccessObjectJsonObj();
    response->get("data") = dataList->value();
    return conn->success(response);
}