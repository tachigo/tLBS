//
// Created by 刘立悟 on 2020/6/4.
//

#include "table.h"
#include "log.h"
#include "t_s2geometry.h"
#include "t_hashmap.h"
#include "common.h"
#include "json.h"
#include "connection.h"
#include "db.h"


using namespace tLBS;

Table::Table(int db, std::string name, unsigned int type,
             unsigned int encoding, void *data) : Object(type, encoding, data) {
    this->db = db;
    this->name = name;
    this->shards = 1;
    this->version = 0;

    this->dirty = 0;
    this->lastSave = time(nullptr);
    this->saving = false;
    this->loading = false;

    char buf[100];
    snprintf(buf, sizeof(buf), " {db#%0.2d(%s)[T:%s][E:%s]} ",
             db, name.c_str(),
             this->getTypeName().c_str(), this->getEncodingName().c_str());
    this->setInfo(buf);
}

Table::~Table() {
    info("销毁") << this->getInfo();
}

int Table::getDb() {
    return this->db;
}

void Table::setDb(int db) {
    // 如果原来的db不为0且和新db值不相等 需要迁移数据文件 todo
    this->db = db;
}

std::string Table::getName() {
    return this->name;
}

int Table::getShards() {
    return this->shards;
}

void Table::setShards(int shards) {
    this->shards = shards;
}

void Table::setVersion(int version) {
    this->version = version;
}

int Table::getVersion() {
    return this->version;
}

void Table::incrDirty(int incr) {
    this->dirty += incr;
}

void Table::decrDirty(int decr) {
    this->dirty -= decr;
}

void Table::resetDirty() {
    this->dirty = 0;
}

int Table::getDirty() {
    return this->dirty;
}

void Table::setSaving(bool saving) {
    this->saving = saving;
}

bool Table::isSaving() {
    return this->saving;
}

time_t Table::getLastSave() {
    return this->lastSave;
}

void Table::setLoading(bool loading) {
    this->loading = loading;
}

bool Table::isLoading() {
    return this->loading;
}

uint64_t Table::getSize() {
    auto data = (TableEncoding *)this->getData();
    return data->getSize();
}

std::string Table::getMetadata() {
    char tmpLine[1024];
    memset(tmpLine, 0, sizeof(tmpLine));
    // dbno/tablename/tabletype/tableencoding/datashardnum/version
    snprintf(tmpLine, sizeof(tmpLine), "%0.2d/%s/%d/%d/%0.2d/%d",
             this->getDb(), this->getName().c_str(), this->getType(),
             this->getEncoding(), this->getShards(), this->getVersion());
    return tmpLine;
}

Table* Table::parseMetadata(std::string metadata) {
//    info(metadata);
    std::vector<std::string> v = splitString(metadata, '/');
    int db = atoi(v[0].c_str());
    std::string name = v[1];
    unsigned int type = atoi(v[2].c_str());
    unsigned int encoding = atoi(v[3].c_str());
    int shards = atoi(v[4].c_str());
    int version = atoi(v[5].c_str());
//    info("parseMetadata: ") << db << "/"
//        << name << "/" << type << "/"
//        << encoding << "/" << shards << "/" << version;


    Table *tableObj = nullptr;
    switch (type) {
        case ObjectType::OBJ_TYPE_GEO_POLYGON :
            switch (encoding) {
                case ObjectEncoding::OBJ_ENCODING_S2GEOMETRY :
                    tableObj = createS2GeoPolygonTable(db, name);
                    break;
                default: break;
            }
            break;
        case ObjectType::OBJ_TYPE_HASH_MAP:
            switch (encoding) {
                case ObjectEncoding::OBJ_ENCODING_STRING_STRING_HASH_MAP:
                    tableObj = createSSHashMapTable(db, name);
                    break;
            }
            break;
        default: break;
    }

    if (tableObj != nullptr) {
        tableObj->setShards(shards);
        tableObj->setVersion(version);
        return tableObj;
    }
    fatal("未知的table的元数据: ") << metadata;
}


void Table::setSaverHandler(tableSaverHandler dumper) {
    this->saverHandler = dumper;
}

int Table::callSaverHandler(std::string dataRootPath) {
    warning("准备将") << this->getInfo() << "数据保存至磁盘";
    if (this->isSaving()) {
        info(this->getInfo()) << "正在保存中，保存结束";
        return C_OK;
    }
    this->setSaving(true);

    int ret = C_OK;

    if (this->saverHandler != nullptr && this->getDirty() > 0) {
        info(this->getInfo()) << "有数据变化, 开始保存";
        ret = this->saverHandler(dataRootPath, this->name, this->getShards(), this->getData());
        if (ret == C_OK) {
            this->resetDirty();
            this->setVersion(this->getVersion() + 1);
            this->lastSave = time(nullptr);
        }
    }

    this->setSaving(false);
    warning(this->getInfo()) << "保存结束";
    return ret;
}

void Table::setLoaderHandler(tLBS::tableLoaderHandler loader) {
    this->loaderHandler = loader;
}

int Table::callLoaderHandler(std::string dataRootPath) {
    warning("准备从磁盘加载") << this->getInfo() << "数据";
    if (this->isLoading()) {
        info(this->getInfo()) << "正在加载中，加载结束";
        return C_OK;
    }
    this->setLoading(true);
    long long start = ustime();
    int ret = C_ERR;

    if (this->loaderHandler != nullptr) {
        ret = this->loaderHandler(dataRootPath, this->name, this->getShards(), this->getData());
    }
    long long duration = ustime() - start;
    char cost[1024];
    memset(cost, 0, sizeof(cost));
    sprintf(cost, "执行时间: %0.5f 毫秒 / %0.5f 秒",
            (double)duration / (double)1000,
            (double)duration / (double)1000000);
    this->setSaving(false);
    warning(this->getInfo()) << "加载结束, " << cost;
    return ret;
}

void Table::setSenderHandler(tLBS::tableSenderHandler sender) {
    this->senderHandler = sender;
}

int Table::callSenderHandler(std::string dataRootPath, std::string prefix, tLBS::Connection *conn) {
    int ret = C_ERR;
    if (this->senderHandler != nullptr) {
        ret = this->senderHandler(dataRootPath, this->name, this->getShards(), this->getData(), prefix, conn);
    }
    return ret;
}

void Table::setReceiverHandler(tLBS::tableReceiverHandler receiver) {
    this->receiverHandler = receiver;
}

int Table::callReceiverHandler(std::string data) {
    int ret = C_ERR;
    if (this->receiverHandler != nullptr) {
        ret = this->receiverHandler(this->getData(), data);
    }
    return ret;
}


Table* Table::createTable(int db, std::string name, unsigned int type, unsigned int encoding, void *data) {
    return new Table(db, name, type, encoding, data);
}

Table * Table::createS2GeoPolygonTable(int db, std::string name) {
    Table *table = createTable(db, name,
                       ObjectType::OBJ_TYPE_GEO_POLYGON,
            ObjectEncoding::OBJ_ENCODING_S2GEOMETRY,
                               new S2Geometry::PolygonIndex());
    table->setSaverHandler(S2Geometry::PolygonIndex::dumper);
    table->setLoaderHandler(S2Geometry::PolygonIndex::loader);
    table->setSenderHandler(S2Geometry::PolygonIndex::sender);
    table->setReceiverHandler(S2Geometry::PolygonIndex::receiver);
    return table;
}

Table* Table::createSSHashMapTable(int db, std::string name) {
    Table *table = createTable(db, name,
                               ObjectType::OBJ_TYPE_HASH_MAP,
                               ObjectEncoding::OBJ_ENCODING_STRING_STRING_HASH_MAP,
                               new HashMap::StringStringHashMap());
    table->setSaverHandler(HashMap::StringStringHashMap::dumper);
    table->setLoaderHandler(HashMap::StringStringHashMap::loader);
    table->setSenderHandler(HashMap::StringStringHashMap::sender);
    table->setReceiverHandler(HashMap::StringStringHashMap::receiver);
    return table;
}



// exec
// tableshards tablename [shardnum]
int Table::execTableShards(Exec *exec, tLBS::Connection *conn, std::vector<std::string> args) {
    UNUSED(exec);
    if (args.size() < 2) {
        return conn->fail(ERRNO_EXEC_SYNTAX_ERR, ERROR_EXEC_SYNTAX_ERR);
    }
    std::string tableName = args[1];
    Table *tableObj = conn->getDb()->lookupTableWrite(tableName);
    if (tableObj == nullptr) {
        return conn->fail(ERRNO_EXEC_TABLE_EXISTS_ERR, ERROR_EXEC_TABLE_EXISTS_ERR);
    }

    if (args.size() == 3) {
        int shards = atoi(args[2].c_str());
        tableObj->setShards(shards);
        exec->setNeedClusterBroadcast(true);
    }
    else {
        Json* json = Json::createSuccessNumberJsonObj();
        json->get("data").SetInt(tableObj->getShards());
        conn->success(json);
        exec->setNeedClusterBroadcast(false);
    }
    return C_OK;
}


int Table::execTables(tLBS::Exec *exec, tLBS::Connection *conn, std::vector<std::string> args) {
    UNUSED(exec);
    UNUSED(args);
    Db *db = conn->getDb();
    std::map<std::string, Table *> tables = db->getTables();

    Json *dataList = new Json(R"({"total": 0, "list": []})");
    dataList->get("total").SetInt(tables.size());
    for (auto mapIter = tables.begin(); mapIter != tables.end(); mapIter++) {
        Json *dataItem = new Json(R"({"metadata": "", "name": "", "db": 0, "shards": 0, "version": 0, "size": 0})");
        auto tableObj = mapIter->second;
        dataItem->get("metadata").SetString(Json::createString(tableObj->getMetadata()));
        dataItem->get("name").SetString(Json::createString(tableObj->getName()));
        dataItem->get("db").SetInt(tableObj->getDb());
        dataItem->get("shards").SetInt(tableObj->getShards());
        dataItem->get("version").SetInt64(tableObj->getVersion());
        dataItem->get("size").SetInt64(tableObj->getSize());
        dataList->get("list").PushBack(dataItem->value(), dataList->getAllocator());
    }
    Json *response = Json::createSuccessObjectJsonObj();
    response->get("data") = dataList->value();
    return conn->success(response);
}