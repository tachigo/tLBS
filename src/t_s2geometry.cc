//
// Created by liuliwu on 2020-06-02.
//

#include "t_s2geometry.h"
#include "common.h"
#include "log.h"
#include "client.h"
#include "db.h"
#include "object.h"

#include <s2/s2text_format.h>

using namespace tLBS;


// commands

int S2Geometry::test(tLBS::Client *client) {
    info("找到我了！");
    return C_OK;
}

// s2polydel table id
int S2Geometry::cmdDelPolygon(tLBS::Client *client) {
    if (client->getArgs().size() != 3) {
        return client->fail(Json::createCmdErrorJsonObj(ERRNO_CMD_SYNTAX_ERR, ERROR_CMD_SYNTAX_ERR));
    }
    Object *tableObj;
    std::string table = client->arg(1);
    std::string id = client->arg(2);

    info("s2polydel: table[") << table << "] "
        << "id[" << id << "] ";
    tableObj = client->getDb()->lookupKeyRead(table);
    if (tableObj == nullptr) {
        info("表`") << table << "`不存在";
        return client->success(Json::createCmdSuccessStringJsonObj());
    }
    else {
        info("表`") << table << "`存在";
        if (tableObj->getType() != OBJ_TYPE_GEO_POLYGON) {
            return client->fail(Json::createCmdErrorJsonObj(ERRNO_CMD_TABLE_TYPE_ERR, ERROR_CMD_TABLE_TYPE_ERR));
        }
        auto indexObj = (S2Geometry::PolygonIndex *) tableObj->getData();
        int shapeId;
        if ((shapeId = indexObj->findShapeIdById(id)) > 0) {
            // 数据存在
            indexObj->delPolygon(shapeId);
            indexObj->flush();
        }
        return client->success(Json::createCmdSuccessStringJsonObj());
    }
    return C_OK; // never reached
}

// s2polyget table id
int S2Geometry::cmdGetPolygon(tLBS::Client *client) {
    if (client->getArgs().size() != 3) {
        return client->fail(Json::createCmdErrorJsonObj(ERRNO_CMD_SYNTAX_ERR, ERROR_CMD_SYNTAX_ERR));
    }
    Object *tableObj;
    std::string table = client->arg(1);
    std::string id = client->arg(2);

    info("s2polyget: table[") << table << "] "
          << "id[" << id << "] ";

    tableObj = client->getDb()->lookupKeyRead(table);
    if (tableObj == nullptr) {
        info("表`") << table << "`不存在";
        return client->success(Json::createCmdSuccessStringJsonObj());
    }
    else {
        info("表`") << table << "`存在";
        if (tableObj->getType() != OBJ_TYPE_GEO_POLYGON) {
            return client->fail(Json::createCmdErrorJsonObj(ERRNO_CMD_TABLE_TYPE_ERR, ERROR_CMD_TABLE_TYPE_ERR));
        }
        auto indexObj = (S2Geometry::PolygonIndex *) tableObj->getData();
        return client->success(Json::createCmdSuccessStringJsonObj(
                indexObj->findDataById(id)
                ));
    }
    return C_OK; // never reached
}


// s2polyset table id data
int S2Geometry::cmdSetPolygon(tLBS::Client *client) {
    if (client->getArgs().size() != 4) {
        return client->fail(Json::createCmdErrorJsonObj(ERRNO_CMD_SYNTAX_ERR, ERROR_CMD_SYNTAX_ERR));
    }
    Object *tableObj;
    std::string table = client->arg(1);
    std::string id = client->arg(2);
    std::string data = client->arg(3);
    info("s2polyset: table[") << table << "] "
        << "id[" << id << "] "
        << "data[" << data << "] ";

    tableObj = client->getDb()->lookupKeyWrite(table);
    if (tableObj == nullptr) {
        info("表`") << table << "`不存在";
        tableObj = Object::createS2GeoPolygonObject(new S2Geometry::PolygonIndex());
        client->getDb()->tableAdd(table, tableObj);
    }
    else {
        info("表`") << table << "`存在";
        if (tableObj->getType() != OBJ_TYPE_GEO_POLYGON) {
            return client->fail(Json::createCmdErrorJsonObj(ERRNO_CMD_TABLE_TYPE_ERR, ERROR_CMD_TABLE_TYPE_ERR));
        }
    }

    int err;
    // 检测多边形是否存在
    auto indexObj = (S2Geometry::PolygonIndex *) tableObj->getData();
    int oldShapeId;
    if ((oldShapeId = indexObj->findShapeIdById(id)) > 0) {
        info("polygon#") << id << "已存在";
        // 删除老的 插入新的
        indexObj->delPolygon(oldShapeId);
        // 新增
        if ((err = indexObj->addPolygon(id, data)) != C_OK) goto err;
    }
    else {
        info("polygon#") << id << "不存在";
        // 新增
        if ((err = indexObj->addPolygon(id, data)) != C_OK) goto err;
    }
    indexObj->flush();
    client->getDb()->incrDirty(1);
    client->success();
    return C_OK;

err:
    client->fail(Json::createCmdErrorJsonObj(err, "添加多边形失败"));
    return C_ERR;
}





// polygon index
S2Geometry::PolygonIndex::PolygonIndex() {
    this->index = new MutableS2ShapeIndex();
}

S2Geometry::PolygonIndex::~PolygonIndex() {
    this->index->Clear();
    delete this->index;
}

int S2Geometry::PolygonIndex::addPolygon(std::string id, std::string data) {
    std::unique_ptr<S2Polygon> polygon;
    S2Debug debugOverride;
    if (!s2textformat::MakePolygon(data.c_str(), &polygon, debugOverride)) {
        return ERRNO_CMD_S2GEOMETRY_ERR;
    }
    int shapeId = this->index->Add(absl::make_unique<S2Polygon::Shape>(polygon.release()));
    this->id2shapeId[id] = shapeId;
    this->shapeId2Data[shapeId] = data;
    return C_OK;
}

void S2Geometry::PolygonIndex::delPolygon(int shapeId) {
    this->shapeId2Data.erase(shapeId); // 清除数据
    delete this->index->Release(shapeId).release();
}

int S2Geometry::PolygonIndex::findShapeIdById(std::string id) {
    auto mapIter = this->id2shapeId.find(id);
    if (mapIter != this->id2shapeId.end()) {
        return mapIter->second;
    }
    return 0;
}

std::string S2Geometry::PolygonIndex::findDataByShapeId(int shapeId) {
    auto mapIter = this->shapeId2Data.find(shapeId);
    if (mapIter != this->shapeId2Data.end()) {
        return mapIter->second;
    }
    return "";
}

std::string S2Geometry::PolygonIndex::findDataById(std::string id) {
    int shapeId = this->findShapeIdById(id);
    return this->findDataByShapeId(shapeId);
}

void S2Geometry::PolygonIndex::flush() {
    this->index->ForceBuild();
}