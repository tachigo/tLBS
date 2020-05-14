//
// Created by liuliwu on 2020-05-13.
//

#include "server.h"
#include "t_polygon.h"
#include "geo.h"
#include <s2/s2polygon.h>
#include <s2/s2text_format.h>

using namespace std;

int polygonAdd(geoPolygonIndex *indexObj, S2Polygon *polygon) {
    int shapeId = indexObj->index->Add(absl::make_unique<S2Polygon::Shape>(polygon));
    return shapeId;
}

void polygonDel(geoPolygonIndex *indexObj, int shapeId) {
    delete indexObj->index->Release(shapeId).release();
}

// polygondel key id
void polygonDelCommand(client *c) {
    obj *tableObj;
    obj *key = c->argv[1];
//    unsigned long long id;
//    getUnsignedLongLongFromObjectOrReply(c, c->argv[2], &id, "id is invalid");
    sds id = (sds) c->argv[2]->ptr;
    serverLog(LL_WARNING, "polygondel: key=%s, id=%s", (char *)key->ptr, id);

    tableObj = lookupKeyWrite(c->db,key);
    if (tableObj == nullptr) {
        serverLog(LL_WARNING, "表`%s`不存在", (char *)key->ptr);
        addReplyError(c, "table not exists");
        return;
    }
    auto indexObj = (geoPolygonIndex *) tableObj->ptr;
    auto mapIter = indexObj->map.find(id);
    if (mapIter == indexObj->map.end()) {
        serverLog(LL_WARNING, "polygon#%s 不存在", id);
        addReplyError(c, "polygon not exists");
        return;
    }
    int shapeId = mapIter->second;
    polygonDel(indexObj, shapeId);
    indexObj->map.erase(id);
    indexObj->data.erase(shapeId);
    server.dirty++;
    addReply(c, shared.ok);
}

// polygonget key id
void polygonGetCommand(client *c) {
    obj *tableObj;
    obj *key = c->argv[1];
//    unsigned long long id;
//    getUnsignedLongLongFromObjectOrReply(c, c->argv[2], &id, "id is invalid");
    sds id = (sds)c->argv[2]->ptr;
    serverLog(LL_WARNING, "polygonget: key=%s, id=%s", (char *)key->ptr, id);
    tableObj = lookupKeyWrite(c->db,key);
    if (tableObj == nullptr) {
        serverLog(LL_WARNING, "表`%s`不存在", (char *)key->ptr);
        addReplyError(c, "table not exists");
        return;
    }
    auto indexObj = (geoPolygonIndex *) tableObj->ptr;
    auto mapIter = indexObj->map.find(id);
    if (mapIter == indexObj->map.end()) {
        serverLog(LL_WARNING, "polygon#%s 不存在", id);
        addReplyError(c, "polygon not exists");
        return;
    }
    int shapeId = mapIter->second;
    string data = indexObj->data[shapeId];
    addReply(c, createStringObject(data.c_str(), data.size()));
}

// polygonset key id s2textformat
void polygonSetCommand(client *c) {
    obj *tableObj;
    obj *key = c->argv[1];
//    unsigned long long id;
//    getUnsignedLongLongFromObjectOrReply(c, c->argv[2], &id, "id is invalid");
    sds id = (sds)c->argv[2]->ptr;
    sds data = (sds)c->argv[3]->ptr;
    serverLog(LL_WARNING, "polygonset: key=%s, id=%s, data=%s", (char *)key->ptr, id, data);

    tableObj = lookupKeyWrite(c->db,key);
    if (tableObj == nullptr) {
        serverLog(LL_WARNING, "表`%s`不存在", (char *)key->ptr);
        // 索引还没有创建
        tableObj = createPolygonObject();
        // 添加进库
        dbAdd(c->db, key, tableObj);
    } else {
        serverLog(LL_WARNING, "表`%s`已存在", (char *)key->ptr);
        if (tableObj->type != OBJ_TYPE_POLYGONS) {
            addReply(c,shared.wrongtypeerr);
            return;
        }
    }

    unique_ptr<S2Polygon> polygon;
    S2Debug debug_override;
    if (!s2textformat::MakePolygon(data, &polygon, debug_override)) {
        addReplyError(c, "-ERR Failed to transform to s2 polygon format.");
        return;
    }
    // 检查这个id的多边形是否已经存在
    auto indexObj = (geoPolygonIndex *) tableObj->ptr;
    auto mapIter = indexObj->map.find(id);
    if (mapIter == indexObj->map.end()) {
        serverLog(LL_WARNING, "polygon#%s 不存在", id);
        // 未存在 新增
        int newShapeId = polygonAdd(indexObj, polygon.release());
        serverLog(LL_WARNING, "polygon#%s 的shape id=%d", id, newShapeId);
        indexObj->map[id] = newShapeId;
        indexObj->data[newShapeId] = data;
        incrRefCount(tableObj);
    } else {
        serverLog(LL_WARNING, "polygon#%s 已存在", id);
        // 已存在 插入新的 删除老的
        int oldShapeId = mapIter->second;
        serverLog(LL_WARNING, "polygon#%s 的旧的shape id=%d", id, oldShapeId);
        int newShapeId = polygonAdd(indexObj, polygon.release());
        serverLog(LL_WARNING, "polygon#%s 的新的shape id=%d", id, newShapeId);
        polygonDel(indexObj, oldShapeId);
        indexObj->map[id] = newShapeId;
        indexObj->data[newShapeId] = data;
    }
    indexObj->index->ForceBuild();
    server.dirty++;
    addReply(c, shared.ok);
}


