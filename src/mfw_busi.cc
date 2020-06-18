//
// Created by liuliwu on 2020-06-08.
//

#include "config.h"
#include "common.h"
#include "mfw_busi.h"
#include "db.h"
#include "command.h"
#include "http.h"
#include "t_hashmap.h"
#include "t_s2geometry.h"

#include <sstream>

using namespace tLBS;

// regionPlaceId和countryRegionPlaceId 的映射关系
#define MFW_REGION2COUNTRY_MAP_DB 0
#define MFW_REGION2COUNTRY_MAP_TABLE "mfw_region2country_tbl"


Table* MfwBusiness::getRegion2CountryTable() {
    Db *db = Db::getDb(MFW_REGION2COUNTRY_MAP_DB);
    Table *tableObj = db->lookupTableWrite(MFW_REGION2COUNTRY_MAP_TABLE);
    if (tableObj == nullptr) {
        tableObj = Table::createSSHashMapTable(MFW_REGION2COUNTRY_MAP_DB, MFW_REGION2COUNTRY_MAP_TABLE);
        db->tableAdd(MFW_REGION2COUNTRY_MAP_TABLE, tableObj);
    }
    return tableObj;
}


#define MFW_REGION_FENCE_TABLE "mfw_region_fence"
// mfwS2RfSet regionPlaceId [regionCountryPlaceId] data
int MfwBusiness::execS2RfSet(tLBS::Exec *exec, tLBS::Connection *conn, std::vector<std::string> args) {
    UNUSED(exec);
    if (args.size() < 3 || args.size() > 4) {
        return conn->fail(ERRNO_EXEC_SYNTAX_ERR, ERROR_EXEC_SYNTAX_ERR);
    }
    std::string regionPlaceId = args[1];
    std::string regionCountryPlaceId;
    std::string data;
    // 通过表去找region的国家place id
    Table *mapTableObj = getRegion2CountryTable();
    auto t = (HashMap::StringStringHashMap *)mapTableObj->getData();
    if (args.size() == 4) {
        std::string regionCountryPlaceId = args[2];
        data = args[3];
    }
    else {
        data = args[2];
        regionCountryPlaceId = t->get(regionPlaceId);
        if (regionCountryPlaceId.empty()) {
            return conn->fail(ERRNO_EXEC_MFW_BUSINESS_ERR, "找不到regionPlaceId对应的countryPlaceId");
        }
    }
    t->set(regionPlaceId, regionCountryPlaceId);

    uint64_t rcId;
    std::istringstream iss(regionCountryPlaceId);
    iss >> rcId;
    int dbId = rcId % FLAGS_db_num;
    Db *db = Db::getDb(dbId);
    Table *tableObj = db->lookupTableWrite(MFW_REGION_FENCE_TABLE);
    if (tableObj == nullptr) {
        tableObj = Table::createS2GeoPolygonTable(db->getId(), MFW_REGION_FENCE_TABLE);
        db->tableAdd(MFW_REGION_FENCE_TABLE, tableObj);
    }
    if (tableObj->getType() != OBJ_TYPE_GEO_POLYGON) {
        return conn->fail(ERRNO_EXEC_TABLE_TYPE_ERR, ERROR_EXEC_TABLE_TYPE_ERR);
    }
    else if (tableObj->getEncoding() != OBJ_ENCODING_S2GEOMETRY) {
        return conn->fail(ERRNO_EXEC_TABLE_ENCODING_ERR, ERROR_EXEC_TABLE_ENCODING_ERR);
    }
    int err;
    // 检测多边形是否存在
    auto indexObj = (S2Geometry::PolygonIndex *) tableObj->getData();
    int oldShapeId;
    if ((oldShapeId = indexObj->findShapeIdById(regionPlaceId)) > 0) {
        // 删除老的 插入新的
        indexObj->delPolygon(oldShapeId);
        // 新增
        if ((err = indexObj->addPolygon(regionPlaceId, data)) != C_OK) goto err;
    }
    else {
        // 新增
        if ((err = indexObj->addPolygon(regionPlaceId, data)) != C_OK) goto err;
    }
    indexObj->flush();
    tableObj->incrDirty(1);
    return conn->success();

err:
    conn->fail(Json::createErrorJsonObj(err, "添加多边形失败"));
    return C_ERR;
}
// mfwS2RfGet regionPlaceId
int MfwBusiness::execS2RfGet(tLBS::Exec *exec, tLBS::Connection *conn, std::vector<std::string> args) {
    UNUSED(exec);
    if (args.size() != 2) {
        return conn->fail(ERRNO_EXEC_SYNTAX_ERR, ERROR_EXEC_SYNTAX_ERR);
    }
    std::string regionPlaceId = args[1];
    // 通过表去找region的国家place id
    Table *mapTableObj = getRegion2CountryTable();
    auto t = (HashMap::StringStringHashMap *)mapTableObj->getData();
    std::string regionCountryPlaceId = t->get(regionPlaceId);
    if (regionCountryPlaceId.empty()) {
        return conn->fail(ERRNO_EXEC_MFW_BUSINESS_ERR, "找不到regionPlaceId对应的countryPlaceId");
    }
    uint64_t rcId;
    std::istringstream iss(regionCountryPlaceId);
    iss >> rcId;
    int dbId = rcId % FLAGS_db_num;
    Db *db = Db::getDb(dbId);
    Table *tableObj = db->lookupTableWrite(MFW_REGION_FENCE_TABLE);
    if (tableObj == nullptr) {
        return conn->success("");
    }
    if (tableObj->getType() != OBJ_TYPE_GEO_POLYGON) {
        return conn->fail(ERRNO_EXEC_TABLE_TYPE_ERR, ERROR_EXEC_TABLE_TYPE_ERR);
    }
    else if (tableObj->getEncoding() != OBJ_ENCODING_S2GEOMETRY) {
        return conn->fail(ERRNO_EXEC_TABLE_ENCODING_ERR, ERROR_EXEC_TABLE_ENCODING_ERR);
    }
    // 检测多边形是否存在
    auto indexObj = (S2Geometry::PolygonIndex *) tableObj->getData();
    std::string data = indexObj->findDataById(regionPlaceId);
    return conn->success(Json::createSuccessStringJsonObj(data.c_str()));
}
// mfwS2RfDel regionPlaceId
int MfwBusiness::execS2RfDel(tLBS::Exec *exec, tLBS::Connection *conn, std::vector<std::string> args) {
    UNUSED(exec);
    if (args.size() != 2) {
        return conn->fail(ERRNO_EXEC_SYNTAX_ERR, ERROR_EXEC_SYNTAX_ERR);
    }
    std::string regionPlaceId = args[1];
    // 通过表去找region的国家place id
    Table *mapTableObj = getRegion2CountryTable();
    auto t = (HashMap::StringStringHashMap *)mapTableObj->getData();
    std::string regionCountryPlaceId = t->get(regionPlaceId);
    if (regionCountryPlaceId.empty()) {
        return conn->fail(ERRNO_EXEC_MFW_BUSINESS_ERR, "找不到regionPlaceId对应的countryPlaceId");
    }
    uint64_t rcId;
    std::istringstream iss(regionCountryPlaceId);
    iss >> rcId;
    int dbId = rcId % FLAGS_db_num;
    Db *db = Db::getDb(dbId);
    Table *tableObj = db->lookupTableWrite(MFW_REGION_FENCE_TABLE);
    if (tableObj == nullptr) {
        return conn->success("");
    }
    if (tableObj->getType() != OBJ_TYPE_GEO_POLYGON) {
        return conn->fail(ERRNO_EXEC_TABLE_TYPE_ERR, ERROR_EXEC_TABLE_TYPE_ERR);
    }
    else if (tableObj->getEncoding() != OBJ_ENCODING_S2GEOMETRY) {
        return conn->fail(ERRNO_EXEC_TABLE_ENCODING_ERR, ERROR_EXEC_TABLE_ENCODING_ERR);
    }
    auto indexObj = (S2Geometry::PolygonIndex *) tableObj->getData();
    int shapeId;
    if ((shapeId = indexObj->findShapeIdById(regionPlaceId)) > 0) {
        // 数据存在
        indexObj->delPolygon(shapeId);
        indexObj->flush();
        tableObj->incrDirty(1);
    }
    return conn->success();
}
// mfwS2RfLocate lat lon
int MfwBusiness::execS2RfLocate(tLBS::Exec *exec, tLBS::Connection *conn, std::vector<std::string> args) {
    UNUSED(exec);
    if (args.size() != 3) {
        return conn->fail(ERRNO_EXEC_SYNTAX_ERR, ERROR_EXEC_SYNTAX_ERR);
    }
    Table *tableObj;
//    std::string table = args[1];
//    tableObj = conn->getDb()->lookupTableRead(table);
//    if (tableObj == nullptr) {
//        return conn->fail(ERRNO_EXEC_TABLE_EXISTS_ERR, ERROR_EXEC_TABLE_EXISTS_ERR);
//    }
//    if (tableObj->getType() != OBJ_TYPE_GEO_POLYGON) {
//        return conn->fail(ERRNO_EXEC_TABLE_TYPE_ERR, ERROR_EXEC_TABLE_TYPE_ERR);
//    }
//    else if (tableObj->getEncoding() != OBJ_ENCODING_S2GEOMETRY) {
//        return conn->fail(ERRNO_EXEC_TABLE_ENCODING_ERR, ERROR_EXEC_TABLE_ENCODING_ERR);
//    }
//    double lat, lon;
//    std::istringstream is1(args[2]);
//    is1 >> lat;
//    std::istringstream is2(args[3]);
//    is2 >> lon;
//    std::vector<std::string> ret;
//    ret.clear();
//    auto indexObj = (S2Geometry::PolygonIndex *) tableObj->getData();
//    indexObj->locPolygon(lat, lon, &ret);
//
//    // 返回数据
//    Json *response = Json::createSuccessArrayJsonObj();
//    for (int i = 0; i < ret.size(); i++) {
//        response->get("data").PushBack(Json::createString(ret[i]), response->getAllocator());
//    }
//    return conn->success(response);
}
// mfwS2RfNearby lat lon distance
int MfwBusiness::execS2RfNearby(tLBS::Exec *exec, tLBS::Connection *conn, std::vector<std::string> args) {
    UNUSED(exec);
    return conn->success();
}

void MfwBusiness::init() {
    // 初始化 regionPlaceId和countryRegionPlaceId 的映射关系 表
    getRegion2CountryTable();
    // 注册exec
    Command::registerCommand("mfwS2RfSet", MfwBusiness::execS2RfSet, "regionPlaceId,regionCountryPlaceId,data", "mfw行政区划数据set", true);
    Http::registerHttp("POST /mfw/s2/region-fence/set", MfwBusiness::execS2RfSet, "regionPlaceId,regionCountryPlaceId,data", "mfw行政区划数据set", false, true);
}

void MfwBusiness::free() {
    
}