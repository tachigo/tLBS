//
// Created by liuliwu on 2020-06-02.
//

#include "t_s2geometry.h"
#include "common.h"
#include "log.h"
#include "connection.h"
#include "db.h"
#include "table.h"

#include <s2/s2text_format.h>
#include <s2/s2closest_edge_query.h>
#include <s2/s2earth.h>
#include <s2/s2contains_point_query.h>
#include <fstream>
#include <sstream>

using namespace tLBS;


// exec

// s2polyclosest table lat lon
int S2Geometry::execPolygonClosest(tLBS::Exec *exec, tLBS::Connection *conn, std::vector<std::string> args) {
    UNUSED(exec);
    if (args.size() != 5) {
        return conn->fail(ERRNO_EXEC_SYNTAX_ERR, ERROR_EXEC_SYNTAX_ERR);
    }
    Table *tableObj;
    std::string table = args[1];
    tableObj = conn->getDb()->lookupTableRead(table);
    if (tableObj == nullptr) {
        return conn->fail(ERRNO_EXEC_TABLE_EXISTS_ERR, ERROR_EXEC_TABLE_EXISTS_ERR);
    }
    if (tableObj->getType() != OBJ_TYPE_GEO_POLYGON) {
        return conn->fail(ERRNO_EXEC_TABLE_TYPE_ERR, ERROR_EXEC_TABLE_TYPE_ERR);
    }
    else if (tableObj->getEncoding() != OBJ_ENCODING_S2GEOMETRY) {
        return conn->fail(ERRNO_EXEC_TABLE_ENCODING_ERR, ERROR_EXEC_TABLE_ENCODING_ERR);
    }
    double lat, lon, distance;
    std::istringstream is1(args[2]);
    is1 >> lat;
    std::istringstream is2(args[3]);
    is2 >> lon;
    std::istringstream is3(args[4]);
    is3 >> distance;
    std::vector<std::map<std::string, std::string>> ret;
    ret.clear();
    auto indexObj = (S2Geometry::PolygonIndex *) tableObj->getData();
    indexObj->closestPolygon(lat, lon, distance, &ret);

    // 返回数据
    Json *response = Json::createSuccessArrayJsonObj();
    for (int i = 0; i < ret.size(); i++) {
        Json *dataItem = new Json(R"({"id": "", "distance": ""})");
        dataItem->get("id").SetString(Json::createString(ret[i]["id"]));
        dataItem->get("distance").SetString(Json::createString(ret[i]["distance"]));
        response->get("data").PushBack(dataItem->value(), response->getAllocator());
    }
    return conn->success(response);
}

// s2polynearby table lat lon distance
int S2Geometry::execPolygonNearby(tLBS::Exec *exec, tLBS::Connection *conn, std::vector<std::string> args) {
    UNUSED(exec);
    if (args.size() != 5) {
        return conn->fail(ERRNO_EXEC_SYNTAX_ERR, ERROR_EXEC_SYNTAX_ERR);
    }
    Table *tableObj;
    std::string table = args[1];
    tableObj = conn->getDb()->lookupTableRead(table);
    if (tableObj == nullptr) {
        return conn->fail(ERRNO_EXEC_TABLE_EXISTS_ERR, ERROR_EXEC_TABLE_EXISTS_ERR);
    }
    if (tableObj->getType() != OBJ_TYPE_GEO_POLYGON) {
        return conn->fail(ERRNO_EXEC_TABLE_TYPE_ERR, ERROR_EXEC_TABLE_TYPE_ERR);
    }
    else if (tableObj->getEncoding() != OBJ_ENCODING_S2GEOMETRY) {
        return conn->fail(ERRNO_EXEC_TABLE_ENCODING_ERR, ERROR_EXEC_TABLE_ENCODING_ERR);
    }
    double lat, lon, distance;
    std::istringstream is1(args[2]);
    is1 >> lat;
    std::istringstream is2(args[3]);
    is2 >> lon;
    std::istringstream is3(args[4]);
    is3 >> distance;

    std::vector<std::map<std::string, std::string>> ret;
    ret.clear();
    auto indexObj = (S2Geometry::PolygonIndex *) tableObj->getData();
    indexObj->nearbyPolygon(lat, lon, distance, &ret);

    // 返回数据
    Json *response = Json::createSuccessArrayJsonObj();
    for (int i = 0; i < ret.size(); i++) {
        Json *dataItem = new Json(R"({"id": "", "distance": ""})");
        dataItem->get("id").SetString(Json::createString(ret[i]["id"]));
        dataItem->get("distance").SetString(Json::createString(ret[i]["distance"]));
        response->get("data").PushBack(dataItem->value(), response->getAllocator());
    }
    return conn->success(response);
}


// s2polyloc table lat lon
int S2Geometry::execPolygonLocate(tLBS::Exec *exec, tLBS::Connection *conn, std::vector<std::string> args) {
    UNUSED(exec);
    if (args.size() != 4) {
        return conn->fail(ERRNO_EXEC_SYNTAX_ERR, ERROR_EXEC_SYNTAX_ERR);
    }
    Table *tableObj;
    std::string table = args[1];
    tableObj = conn->getDb()->lookupTableRead(table);
    if (tableObj == nullptr) {
        return conn->fail(ERRNO_EXEC_TABLE_EXISTS_ERR, ERROR_EXEC_TABLE_EXISTS_ERR);
    }
    if (tableObj->getType() != OBJ_TYPE_GEO_POLYGON) {
        return conn->fail(ERRNO_EXEC_TABLE_TYPE_ERR, ERROR_EXEC_TABLE_TYPE_ERR);
    }
    else if (tableObj->getEncoding() != OBJ_ENCODING_S2GEOMETRY) {
        return conn->fail(ERRNO_EXEC_TABLE_ENCODING_ERR, ERROR_EXEC_TABLE_ENCODING_ERR);
    }
    double lat, lon;
    std::istringstream is1(args[2]);
    is1 >> lat;
    std::istringstream is2(args[3]);
    is2 >> lon;
    std::vector<std::string> ret;
    ret.clear();
    auto indexObj = (S2Geometry::PolygonIndex *) tableObj->getData();
    indexObj->locatePolygon(lat, lon, &ret);

    // 返回数据
    Json *response = Json::createSuccessArrayJsonObj();
    for (int i = 0; i < ret.size(); i++) {
        response->get("data").PushBack(Json::createString(ret[i]), response->getAllocator());
    }
    return conn->success(response);
}

// s2build table
int S2Geometry::execForceBuild(Exec *exec, tLBS::Connection *conn, std::vector<std::string> args) {
    UNUSED(exec);
    if (args.size() != 2) {
        return conn->fail(ERRNO_EXEC_SYNTAX_ERR, ERROR_EXEC_SYNTAX_ERR);
    }
    Table *tableObj;
    std::string table = args[1];

    tableObj = conn->getDb()->lookupTableWrite(table);
    if (tableObj == nullptr) {
        return conn->fail(ERRNO_EXEC_TABLE_EXISTS_ERR, ERROR_EXEC_TABLE_EXISTS_ERR);
    }
    else {
        if (tableObj->getType() != OBJ_TYPE_GEO_POLYGON) {
            return conn->fail(ERRNO_EXEC_TABLE_TYPE_ERR, ERROR_EXEC_TABLE_TYPE_ERR);
        }
        else if (tableObj->getEncoding() != OBJ_ENCODING_S2GEOMETRY) {
            return conn->fail(ERRNO_EXEC_TABLE_ENCODING_ERR, ERROR_EXEC_TABLE_ENCODING_ERR);
        }
        auto indexObj = (S2Geometry::PolygonIndex *) tableObj->getData();
        indexObj->flush();
        return conn->success();
    }
    return C_OK; // never reached
}

// s2polydel table id
int S2Geometry::execPolygonDel(Exec *exec, Connection *conn, std::vector<std::string> args) {
    UNUSED(exec);
    if (args.size() != 3) {
        return conn->fail(ERRNO_EXEC_SYNTAX_ERR, ERROR_EXEC_SYNTAX_ERR);
    }
    Table *tableObj;
    std::string table = args[1];
    std::string id = args[2];

//    info("s2polydel: table[") << table << "] "
//        << "id[" << id << "] ";

    tableObj = conn->getDb()->lookupTableWrite(table);
    if (tableObj == nullptr) {
//        info("表`") << table << "`不存在";
        return conn->success();
    }
    else {
//        info("表`") << table << "`存在";
        if (tableObj->getType() != OBJ_TYPE_GEO_POLYGON) {
            return conn->fail(ERRNO_EXEC_TABLE_TYPE_ERR, ERROR_EXEC_TABLE_TYPE_ERR);
        }
        else if (tableObj->getEncoding() != OBJ_ENCODING_S2GEOMETRY) {
            return conn->fail(ERRNO_EXEC_TABLE_ENCODING_ERR, ERROR_EXEC_TABLE_ENCODING_ERR);
        }
        auto indexObj = (S2Geometry::PolygonIndex *) tableObj->getData();
        int shapeId;
        if ((shapeId = indexObj->findShapeIdById(id)) > 0) {
            // 数据存在
            indexObj->delPolygon(shapeId);
            indexObj->flush();
            tableObj->incrDirty(1);
        }
        return conn->success();
    }
    return C_OK; // never reached
}

// s2polyget table id
int S2Geometry::execPolygonGet(Exec *exec, Connection *conn, std::vector<std::string> args) {
    UNUSED(exec);
    if (args.size() != 3) {
        return conn->fail(ERRNO_EXEC_SYNTAX_ERR, ERROR_EXEC_SYNTAX_ERR);
    }
    Table *tableObj;
    std::string table = args[1];
    std::string id = args[2];

//    info("s2polyget: table[") << table << "] "
//          << "id[" << id << "] ";

    tableObj = conn->getDb()->lookupTableRead(table);
    if (tableObj == nullptr) {
//        info("表`") << table << "`不存在";
        return conn->success(Json::createSuccessStringJsonObj());
    }
    else {
//        info("表`") << table << "`存在";
        if (tableObj->getType() != OBJ_TYPE_GEO_POLYGON) {
            return conn->fail(ERRNO_EXEC_TABLE_TYPE_ERR, ERROR_EXEC_TABLE_TYPE_ERR);
        }
        auto indexObj = (S2Geometry::PolygonIndex *) tableObj->getData();
        std::string data = indexObj->findDataById(id);
        return conn->success(Json::createSuccessStringJsonObj(data.c_str()));
    }
    return C_OK; // never reached
}


// s2polyset table id data
int S2Geometry::execPolygonSet(Exec *exec, Connection *conn, std::vector<std::string> args) {
    UNUSED(exec);
    if (args.size() != 4) {
        return conn->fail(ERRNO_EXEC_SYNTAX_ERR, ERROR_EXEC_SYNTAX_ERR);
    }
    Table *tableObj;
    std::string table = args[1];
    std::string id = args[2];
    std::string data = args[3];
//    info("s2polyset: table[") << table << "] "
//        << "id[" << id << "] "
//        << "data[" << data << "] ";

    tableObj = conn->getDb()->lookupTableWrite(table);
    if (tableObj == nullptr) {
//        info("表`") << table << "`不存在";
        tableObj = Table::createS2GeoPolygonTable(
                conn->getDb()->getId(),
                table);
        conn->getDb()->tableAdd(table, tableObj);
    }
    else {
//        info("表`") << table << "`存在";
        if (tableObj->getType() != OBJ_TYPE_GEO_POLYGON) {
            return conn->fail(ERRNO_EXEC_TABLE_TYPE_ERR, ERROR_EXEC_TABLE_TYPE_ERR);
        }
    }

    int err;
    // 检测多边形是否存在
    auto indexObj = (S2Geometry::PolygonIndex *) tableObj->getData();
    int oldShapeId;
    if ((oldShapeId = indexObj->findShapeIdById(id)) > 0) {
        // 删除老的 插入新的
        indexObj->delPolygon(oldShapeId);
        // 新增
        if ((err = indexObj->addPolygon(id, data)) != C_OK) goto err;
    }
    else {
        // 新增
        if ((err = indexObj->addPolygon(id, data)) != C_OK) goto err;
    }
    indexObj->flush();
    tableObj->incrDirty(1);
    conn->success();
    return C_OK;

err:
    conn->fail(Json::createErrorJsonObj(err, "添加多边形失败"));
    return C_ERR;
}

int S2Geometry::PolygonIndex::dump(std::string dataRootPath, std::string table, int shards) {
    std::ofstream ofs[shards];
    for (int i = 0; i < shards; i++) {
        std::string tmpFile = dataRootPath + this->getTmpFile(table, i);
        ofs[i].open(tmpFile, std::ios::out | std::ios::trunc);
    }

    for (auto mapIter = this->id2shapeId.begin(); mapIter != this->id2shapeId.end(); mapIter++) {
        std::string id = mapIter->first;
        int shapeId = mapIter->second;
        std::string data = this->shapeId2Data.find(shapeId)->second;
        int shard = shapeId % shards;
        ofs[shard] << id << "+" << data << std::endl;
    }

    for (int i = 0; i < shards; i++) {
        ofs[i].close();
        std::string tmpFile = dataRootPath + this->getTmpFile(table, i);
        std::string datFile = dataRootPath + this->getDatFile(table, i);
        if (rename(tmpFile.c_str(), datFile.c_str()) == -1) {
            error("将临时文件") << tmpFile << "移动到最终文件" << datFile << "失败!";
            unlink(tmpFile.c_str());
            goto err;
        }
    }
    return C_OK;

err:
    for (int i = 0; i < shards; i++) {
        std::string tmpFile = dataRootPath + this->getTmpFile(table, i);
        ofs[i].close();
        unlink(tmpFile.c_str());
    }
    return C_ERR;
}

int S2Geometry::PolygonIndex::send(std::string dataRootPath, std::string table, int shards, std::string prefix, tLBS::Connection *conn) {
    std::ifstream ifs;
    for (int i = 0; i < shards; i++) {
        std::string datFile = dataRootPath + this->getDatFile(table, i);
        ifs.open(datFile, std::ios::in);
        if (!ifs) {
            if (errno == ENOENT) {
                // 文件不存在
                error("无法打开磁盘数据文件" + datFile)
                        << ": " << strerror(errno) << "(" << errno << ")";
            }
            else {
                fatal("无法打开磁盘数据文件" + datFile)
                        << ": " << strerror(errno) << "(" << errno << ")";
            }
        }
        else {
            std::string line;
            while (getline(ifs, line)) {
                line = prefix + " " + line + '\n'; // 每行增加换行符
                conn->write(line.c_str(), line.size());
            }
            ifs.close();
        }
    }
    return C_OK;
}


int S2Geometry::PolygonIndex::receive(std::string line) {
    std::vector<std::string> v = splitString(line, '+');
    std::string id = v[0];
    std::string data = v[1];

    int oldShapeId;
    if ((oldShapeId = this->findShapeIdById(id)) > 0) {
        // 删除老的 插入新的
        this->delPolygon(oldShapeId);
        // 新增
        if (this->addPolygon(id, data) != C_OK) return C_ERR;
    }
    else {
        // 新增
        if (this->addPolygon(id, data) != C_OK) return C_ERR;
    }
    this->flush();
    return C_OK;
}

int S2Geometry::PolygonIndex::load(std::string dataRootPath, std::string table, int shards) {
    std::ifstream ifs;
    for (int i = 0; i < shards; i++) {
        std::string datFile = dataRootPath + this->getDatFile(table, i);
        ifs.open(datFile, std::ios::in);
        if (!ifs) {
            if (errno == ENOENT) {
                // 文件不存在
                error("无法打开磁盘数据文件" + datFile)
                        << ": " << strerror(errno) << "(" << errno << ")";
            }
            else {
                fatal("无法打开磁盘数据文件" + datFile)
                        << ": " << strerror(errno) << "(" << errno << ")";
            }
        }
        else {
            std::string line;
            while (getline(ifs, line)) {
                std::vector<std::string> v = splitString(line, '+');
                std::string id = v[0];
                std::string data = v[1];
                if (this->addPolygon(id, data) != C_OK) {
                    fatal("加载数据失败: ") << line;
                }
//                info(table) << " load: " << id;
            }
            this->flush();
            ifs.close();
        }
    }
    return C_OK;
}

std::string S2Geometry::PolygonIndex::getTmpFile(std::string table, int shard) {
    char tmpFile[1024];
    snprintf(tmpFile, sizeof(tmpFile), "s2polygon<%s>-%0.2d-%d.tmp", table.c_str(), shard, ::getpid());
    return tmpFile;
}

std::string S2Geometry::PolygonIndex::getDatFile(std::string table, int shard) {
    char datFile[1024];
    snprintf(datFile, sizeof(datFile),"s2polygon<%s>-%0.2d.dat", table.c_str(), shard);
    return datFile;
}


int S2Geometry::PolygonIndex::dumper(std::string dataRootPath, std::string table, int shards, void *ptr) {
    auto data = (PolygonIndex *)ptr;
    return data->dump(dataRootPath, table, shards);
}


int S2Geometry::PolygonIndex::loader(std::string dataRootPath, std::string table, int shards, void *ptr) {
    auto data = (PolygonIndex *)ptr;
    return data->load(dataRootPath, table, shards);
}

int S2Geometry::PolygonIndex::sender(std::string dataRootPath, std::string table, int shards, void *ptr, std::string prefix, Connection *conn) {
    auto data = (PolygonIndex *)ptr;
    return data->send(dataRootPath, table, shards, prefix, conn);
}

int S2Geometry::PolygonIndex::receiver(void *ptr, std::string line) {
    auto data = (PolygonIndex *)ptr;
    return data->receive(line);
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
        return ERRNO_EXEC_S2GEOMETRY_ERR;
    }
    int shapeId = this->index->Add(absl::make_unique<S2Polygon::Shape>(polygon.release()));
    this->id2shapeId[id] = shapeId;
    this->shapeId2id[shapeId] = id;
    this->shapeId2Data[shapeId] = data;
    return C_OK;
}

void S2Geometry::PolygonIndex::delPolygon(int shapeId) {
    this->shapeId2Data.erase(shapeId);
    this->shapeId2id.erase(shapeId);
    delete this->index->Release(shapeId).release();
}

int S2Geometry::PolygonIndex::locatePolygon(double lat, double lon, std::vector<std::string> *ret) {
    S2LatLng latlng = S2LatLng::FromDegrees(lat, lon);
    S2ContainsPointQuery<MutableS2ShapeIndex> query = MakeS2ContainsPointQuery(this->index,S2ContainsPointQueryOptions(S2VertexModel::CLOSED));
    for (S2Shape* shape : query.GetContainingShapes(latlng.ToPoint())) {
        info(shape->id());
        std::string id = this->findIdByShapeId(shape->id());
        if (id.size() > 0) {
            ret->push_back(id);
        }
    }
    return C_OK;
}

int S2Geometry::PolygonIndex::closestPolygon(double lat, double lon, double distance, std::vector<std::map<std::string, std::string>> *ret) {
    S2LatLng latlng = S2LatLng::FromDegrees(lat, lon);
    S2ClosestEdgeQuery query(this->index);
    S2ClosestEdgeQuery::PointTarget target(latlng.ToPoint());
    S2ClosestEdgeQuery::Result result = query.FindClosestEdge(&target);
    if (result.shape_id() > 0) {
        if (distance > 0 && S2Earth::ToKm(result.distance()) > distance) {
            return C_OK;
        }
        std::string id = this->findIdByShapeId(result.shape_id());
        std::map<std::string, std::string> map;
        map.clear();
        map["id"] = id;
        map["distance"] = S2Earth::ToKm(result.distance());
        ret->push_back(map);
    }
    return C_OK;
}

int S2Geometry::PolygonIndex::nearbyPolygon(double lat, double lon, double distance, std::vector<std::map<std::string, std::string>> *ret) {
    S2LatLng latlng = S2LatLng::FromDegrees(lat, lon);
    S2ClosestEdgeQuery query(this->index);
    query.mutable_options()->set_max_results(500);
    query.mutable_options()->set_max_distance(S2Earth::ToAngle(util::units::Kilometers(distance)));
    query.mutable_options()->set_include_interiors(false);
    S2ClosestEdgeQuery::PointTarget target(latlng.ToPoint());
    std::map<std::string, int> record;
    for(const auto& result : query.FindClosestEdges(&target)){
        std::string id = this->findIdByShapeId(result.shape_id());
        if (!id.empty()) {
            if (record.find(id) == record.end()) {
                // 没有加入过
                std::map<std::string, std::string> map;
                map.clear();
                map["id"] = id;
                map["distance"] = S2Earth::ToKm(result.distance());
                ret->push_back(map);
                record[id] = 1;
            }
        }
    }
    return C_OK;
}

std::string S2Geometry::PolygonIndex::findIdByShapeId(int shapeId) {
    auto mapIter = this->shapeId2id.find(shapeId);
    if (mapIter != this->shapeId2id.end()) {
        return mapIter->second;
    }
    return "";
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

uint64_t S2Geometry::PolygonIndex::getSize() {
    return this->id2shapeId.size();
}