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

std::string Table::getMetadata() {
    char tmpLine[1024];
    memset(tmpLine, 0, sizeof(tmpLine));
    // dbno/tablename/tabletype/tableencoding/datashardnum
    snprintf(tmpLine, sizeof(tmpLine), "%0.2d/%s/%d/%d/%0.2d",
             this->getDb(), this->getName().c_str(), this->getType(),
             this->getEncoding(), this->getShards());
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
    info("parseMetadata: ") << db << "/"
        << name << "/" << type << "/"
        << encoding << "/" << shards;
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
        return tableObj;
    }
    fatal("未知的table的元数据: ") << metadata;
}


void Table::setDumperHandler(tLBS::tableDumperHandler dumper) {
    this->dumperHandler = dumper;
}

int Table::callDumperHandler(std::string dataRootPath) {
    if (this->dumperHandler != nullptr) {
        info(this->getInfo()) << " 有dumper句柄, 开始执行dump";
        return this->dumperHandler(dataRootPath, this->name, this->getShards(), this->getData());
    }
    return C_OK;
}

void Table::setLoaderHandler(tLBS::tableLoaderHandler loader) {
    this->loaderHandler = loader;
}

int Table::callLoaderHandler(std::string dataRootPath) {
    if (this->loaderHandler != nullptr) {
        info(this->getInfo()) << " 有loader句柄, 开始执行load";
        return this->loaderHandler(dataRootPath, this->name, this->getShards(), this->getData());
    }
    return C_OK;
}


Table* Table::createTable(int db, std::string name, unsigned int type, unsigned int encoding, void *data) {
    return new Table(db, name, type, encoding, data);
}

Table * Table::createS2GeoPolygonTable(int db, std::string name) {
    Table *table = createTable(db, name,
                       ObjectType::OBJ_TYPE_GEO_POLYGON,
            ObjectEncoding::OBJ_ENCODING_S2GEOMETRY,
                               new S2Geometry::PolygonIndex());
    table->setDumperHandler(S2Geometry::PolygonIndex::dumper);
    table->setLoaderHandler(S2Geometry::PolygonIndex::loader);
    return table;
}