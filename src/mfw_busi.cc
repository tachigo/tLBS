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

// 国家级区划表
#define MFW_COUNTRY_REGION_FENCE_DB 0
#define MFW_COUNTRY_REGION_FENCE_TABLE "mfw_country_region_fence_tbl"
Table* MfwBusiness::getCountryRegionFenceTable() {
    Db *db = Db::getDb(MFW_COUNTRY_REGION_FENCE_DB);
    Table *tableObj = db->lookupTableWrite(MFW_COUNTRY_REGION_FENCE_TABLE);
    if (tableObj == nullptr) {
        tableObj = Table::createSSHashMapTable(MFW_COUNTRY_REGION_FENCE_DB, MFW_COUNTRY_REGION_FENCE_TABLE);
        db->tableAdd(MFW_COUNTRY_REGION_FENCE_TABLE, tableObj);
    }
    return tableObj;
}

// 行政区划region_fence表
#define MFW_REGION_FENCE_TABLE "mfw_region_fence_tbl"
#define MFW_REGION_COUNTRY_HASH_SLOT 50
// mfwS2RfSet regionPlaceId [regionCountryPlaceId] data
int MfwBusiness::execS2RfSet(tLBS::Exec *exec, tLBS::Connection *conn, std::vector<std::string> args) {
    UNUSED(exec);
    if (args.size() < 3 || args.size() > 4) {
        return conn->fail(ERRNO_EXEC_SYNTAX_ERR, ERROR_EXEC_SYNTAX_ERR);
    }
    std::string regionPlaceId = args[1];
    std::string regionCountryPlaceId;
    std::string data;
    int err;
    int oldShapeId;
    Table *tableObj;
    S2Geometry::PolygonIndex *indexObj;
    Db *db;
    int dbId;
    // 通过表去找region的国家place id
    tableObj = getRegion2CountryTable();
    auto hash = (HashMap::StringStringHashMap *)tableObj->getData();
    if (args.size() == 4) {
        regionCountryPlaceId = args[2];
        data = args[3];
    }
    else {
        data = args[2];
        regionCountryPlaceId = hash->get(regionPlaceId);
        if (regionCountryPlaceId.empty()) {
            return conn->fail(ERRNO_EXEC_MFW_BUSINESS_ERR, "找不到regionPlaceId对应的countryPlaceId");
        }
    }
    hash->set(regionPlaceId, regionCountryPlaceId);

    uint64_t rcId;
    std::istringstream iss(regionCountryPlaceId);
    iss >> rcId;
    dbId = (rcId % MFW_REGION_COUNTRY_HASH_SLOT) % FLAGS_db_num;

    if (regionPlaceId == regionCountryPlaceId) {
        // 如果就是国家级的行政区划
        tableObj = getCountryRegionFenceTable();
        // 检测多边形是否存在
        indexObj = (S2Geometry::PolygonIndex *) tableObj->getData();
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
    }

    db = Db::getDb(dbId);
    tableObj = db->lookupTableWrite(MFW_REGION_FENCE_TABLE);
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
    // 检测多边形是否存在
    indexObj = (S2Geometry::PolygonIndex *) tableObj->getData();
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
    auto hash = (HashMap::StringStringHashMap *)mapTableObj->getData();
    std::string regionCountryPlaceId = hash->get(regionPlaceId);
    if (regionCountryPlaceId.empty()) {
        return conn->fail(ERRNO_EXEC_MFW_BUSINESS_ERR, "找不到regionPlaceId对应的countryPlaceId");
    }
    uint64_t rcId;
    std::istringstream iss(regionCountryPlaceId);
    iss >> rcId;
    int dbId = (rcId % MFW_REGION_COUNTRY_HASH_SLOT) % FLAGS_db_num;
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
    int dbId = (rcId % MFW_REGION_COUNTRY_HASH_SLOT) % FLAGS_db_num;
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
    S2Geometry::PolygonIndex *indexObj;
    Db *db;
    int dbId;
    double lat, lon;
    std::istringstream is1(args[1]);
    is1 >> lat;
    std::istringstream is2(args[2]);
    is2 >> lon;
    std::vector<std::string> ret;
    ret.clear();
    // 返回数据
    Json *response = Json::createSuccessArrayJsonObj();
    // 先进行国家定位
    tableObj = getCountryRegionFenceTable();
    indexObj = (S2Geometry::PolygonIndex *) tableObj->getData();
    indexObj->locatePolygon(lat, lon, &ret);
    if (ret.empty()) {
        // 没有定位到国际的话就走最近逻辑
        return conn->success(response);
    }
    std::string regionCountryPlaceId = ret[0];
    ret.clear();
    // 再根据国家定位hash到对应的表进行定位
    uint64_t rcId;
    std::istringstream iss(regionCountryPlaceId);
    iss >> rcId;
    dbId = (rcId % MFW_REGION_COUNTRY_HASH_SLOT) % FLAGS_db_num;
    db = Db::getDb(dbId);
    tableObj = db->lookupTableWrite(MFW_REGION_FENCE_TABLE);
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
    indexObj = (S2Geometry::PolygonIndex *) tableObj->getData();
    indexObj->locatePolygon(lat, lon, &ret);

    for (int i = 0; i < ret.size(); i++) {
        response->get("data").PushBack(Json::createString(ret[i]), response->getAllocator());
    }
    return conn->success(response);
}
// 寻找附近
// mfwS2RfNearby lat lon distance
int MfwBusiness::execS2RfNearby(tLBS::Exec *exec, tLBS::Connection *conn, std::vector<std::string> args) {
    UNUSED(exec);
    if (args.size() != 4) {
        return conn->fail(ERRNO_EXEC_SYNTAX_ERR, ERROR_EXEC_SYNTAX_ERR);
    }
    Table *tableObj;
    S2Geometry::PolygonIndex *indexObj;
    Db *db;
    int dbId;
    double lat, lon, distance;
    std::istringstream is1(args[1]);
    is1 >> lat;
    std::istringstream is2(args[2]);
    is2 >> lon;
    std::istringstream is3(args[3]);
    is3 >> distance;
    std::vector<std::map<std::string, std::string>> countryRet;
    Json *response = Json::createSuccessArrayJsonObj();
    // 先确定点半径范围内的国家
    tableObj = getCountryRegionFenceTable();
    indexObj = (S2Geometry::PolygonIndex *) tableObj->getData();
    indexObj->nearbyPolygon(lat, lon, distance, &countryRet);
    if (countryRet.empty()) {
        return conn->success(response);
    }
    // 再按照各个国家查找点半径范围内的数据
    for (int i = 0; i < countryRet.size(); i++) {
        std::string regionCountryPlaceId = countryRet[i]["id"];
        uint64_t rcId;
        std::istringstream iss(regionCountryPlaceId);
        iss >> rcId;
        dbId = (rcId % MFW_REGION_COUNTRY_HASH_SLOT) % FLAGS_db_num;
        db = Db::getDb(dbId);
        tableObj = db->lookupTableWrite(MFW_REGION_FENCE_TABLE);
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
        std::vector<std::map<std::string, std::string>> ret;
        ret.clear();
        indexObj = (S2Geometry::PolygonIndex *) tableObj->getData();
        indexObj->nearbyPolygon(lat, lon, distance, &ret);
        for (int i = 0; i < ret.size(); i++) {
            Json *dataItem = new Json(R"({"id": "", "distance": ""})");
            dataItem->get("id").SetString(Json::createString(ret[i]["id"]));
            dataItem->get("distance").SetString(Json::createString(ret[i]["distance"]));
            response->get("data").PushBack(dataItem->value(), response->getAllocator());
        }
    }
    return conn->success(response);
}

// mfwS2RfClosest lat lon distance
int MfwBusiness::execS2RfClosest(tLBS::Exec *exec, tLBS::Connection *conn, std::vector<std::string> args) {
    UNUSED(exec);
    if (args.size() != 4) {
        return conn->fail(ERRNO_EXEC_SYNTAX_ERR, ERROR_EXEC_SYNTAX_ERR);
    }
    Table *tableObj;
    S2Geometry::PolygonIndex *indexObj;
    Db *db;
    int dbId;
    double lat, lon, distance;
    std::istringstream is1(args[1]);
    is1 >> lat;
    std::istringstream is2(args[2]);
    is2 >> lon;
    std::istringstream is3(args[3]);
    is3 >> distance;
    std::vector<std::map<std::string, std::string>> countryRet;
    Json *response = Json::createSuccessArrayJsonObj();
    // 先找最近的国家
    tableObj = getCountryRegionFenceTable();
    indexObj = (S2Geometry::PolygonIndex *) tableObj->getData();
    indexObj->closestPolygon(lat, lon, distance, &countryRet);
    if (countryRet.empty()) {
        return conn->success(response);
    }
    for (int i = 0; i < countryRet.size(); i++) {
        std::string regionCountryPlaceId = countryRet[0]["id"];
        // 再根据国家定位hash到对应的表进行定位
        uint64_t rcId;
        std::istringstream iss(regionCountryPlaceId);
        iss >> rcId;
        dbId = (rcId % MFW_REGION_COUNTRY_HASH_SLOT) % FLAGS_db_num;
        db = Db::getDb(dbId);
        tableObj = db->lookupTableWrite(MFW_REGION_FENCE_TABLE);
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
        std::vector<std::map<std::string, std::string>> ret;
        ret.clear();
        indexObj = (S2Geometry::PolygonIndex *) tableObj->getData();
        // 增加一点距离查询点半径附近
        indexObj->nearbyPolygon(lat, lon, distance + 1, &ret);
        for (int j = 0; j < ret.size(); j++) {
            Json *dataItem = new Json(R"({"id": "", "distance": ""})");
            dataItem->get("id").SetString(Json::createString(ret[j]["id"]));
            dataItem->get("distance").SetString(Json::createString(ret[j]["distance"]));
            response->get("data").PushBack(dataItem->value(), response->getAllocator());
        }
    }
    return conn->success(response);
}

void MfwBusiness::init() {
    // 初始化 regionPlaceId和countryRegionPlaceId 的映射关系 表
    getRegion2CountryTable();
    // 注册exec
    Command::registerCommand("mfwS2RfSet", MfwBusiness::execS2RfSet, "regionPlaceId,regionCountryPlaceId,data", "mfw行政区划数据set", true);
    Command::registerCommand("mfwS2RfGet", MfwBusiness::execS2RfGet, "regionPlaceId", "mfw行政区划数据get", false);
    Command::registerCommand("mfwS2RfDel", MfwBusiness::execS2RfDel, "regionPlaceId", "mfw行政区划数据del", true);
    Command::registerCommand("mfwS2RfLocate", MfwBusiness::execS2RfLocate, "lat,lon", "mfw行政区划数据定位", false);
    Command::registerCommand("mfwS2RfNearby", MfwBusiness::execS2RfNearby, "lat,lon,distance", "mfw行政区划数据点半径查询附近", false);
    Command::registerCommand("mfwS2RfClosest", MfwBusiness::execS2RfClosest, "lat,lon,distance", "mfw行政区划数据点半径查询最近", false);
    Http::registerHttp("POST /mfw/s2/region-fence/set", MfwBusiness::execS2RfSet, "regionPlaceId,regionCountryPlaceId,data", "mfw行政区划数据set", false, true);
    Http::registerHttp("GET /mfw/s2/region-fence/get", MfwBusiness::execS2RfGet, "regionPlaceId", "mfw行政区划数据get", false, false);
    Http::registerHttp("GET /mfw/s2/region-fence/del", MfwBusiness::execS2RfGet, "regionPlaceId", "mfw行政区划数据del", false, true);
    Http::registerHttp("GET /mfw/s2/region-fence/locate", MfwBusiness::execS2RfLocate, "lat,lon", "mfw行政区划数据定位", false, false);
    Http::registerHttp("GET /mfw/s2/region-fence/nearby", MfwBusiness::execS2RfNearby, "lat,lon,distance", "mfw行政区划数据点半径查询附近", false, false);
    Http::registerHttp("GET /mfw/s2/region-fence/closest", MfwBusiness::execS2RfClosest, "lat,lon,distance", "mfw行政区划数据点半径查询最近", false, false);
}

void MfwBusiness::free() {
    
}