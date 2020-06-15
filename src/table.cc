//
// Created by 刘立悟 on 2020/6/4.
//

#include <regex>
#include "table.h"
#include "log.h"
#include "t_s2geometry.h"
#include "common.h"

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
    snprintf(buf, sizeof(buf), "db#%0.2d`%s`[T:%s][E:%s]",
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

std::string Table::getMetadata() {
    int version = this->getVersion();
    if (this->getDirty() > 0) {
        version = version + 1;
    }
    char tmpLine[1024];
    memset(tmpLine, 0, sizeof(tmpLine));
    // dbno/tablename/tabletype/tableencoding/datashardnum/version
    snprintf(tmpLine, sizeof(tmpLine), "%0.2d/%s/%d/%d/%0.2d/%d",
             this->getDb(), this->getName().c_str(), this->getType(),
             this->getEncoding(), this->getShards(), version);
    return tmpLine;
}

Table* Table::parseMetadata(std::string metadata) {
    info(metadata);
    std::regex reg("/");
    std::vector<std::string> v(
            std::sregex_token_iterator(
                    metadata.begin(), metadata.end(), reg, -1
                    ),
            std::sregex_token_iterator());
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
    switch (encoding) {
        case ObjectEncoding::OBJ_ENCODING_S2GEOMETRY :
            switch (type) {
                case ObjectType::OBJ_TYPE_GEO_POLYGON :
                    tableObj = createS2GeoPolygonTable(db, name);
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

    int ret = C_ERR;

    if (this->saverHandler != nullptr && this->getDirty() > 0) {
        info(this->getInfo()) << "有数据变化, 开始保存";
        ret = this->saverHandler(dataRootPath, this->name, this->getShards(), this->getData());
        if (ret == C_OK) {
            this->resetDirty();
        }
    }

    this->setSaving(false);
    warning(this->getInfo()) << "加载结束";
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

    int ret = C_ERR;

    if (this->loaderHandler != nullptr) {
        ret = this->loaderHandler(dataRootPath, this->name, this->getShards(), this->getData());
    }
    this->setSaving(false);
    warning(this->getInfo()) << "加载结束";
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
    return table;
}