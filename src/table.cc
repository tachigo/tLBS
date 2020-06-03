//
// Created by 刘立悟 on 2020/6/4.
//

#include "table.h"
#include "log.h"

using namespace tLBS;

Table::Table(int db, std::string name, unsigned int type,
             unsigned int encoding, void *data) : Object(type, encoding, data) {
    this->db = db;
    this->name = name;

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


Table* Table::createTable(int db, std::string name, unsigned int type, unsigned int encoding, void *data) {
    return new Table(db, name, type, encoding, data);
}

Table * Table::createS2GeoPolygonTable(int db, std::string name, void *data) {
    return createTable(db, name,
                       ObjectType::OBJ_TYPE_GEO_POLYGON,
            ObjectEncoding::OBJ_ENCODING_S2GEOMETRY,
            data);
}