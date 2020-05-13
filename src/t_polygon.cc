//
// Created by liuliwu on 2020-05-13.
//

#include "server.h"
#include "t_polygon.h"
#include "s2.h"
#include <s2/s2polygon.h>
#include <s2/s2text_format.h>

using namespace std;

int polygonAdd(s2polygonIndex *indexObj, S2Polygon *polygon) {
    int shapeId = indexObj->index->Add(absl::make_unique<S2Polygon::Shape>(polygon));
    return shapeId;
}

void polygonDel(s2polygonIndex *indexObj, S2Polygon *polygon, int shapeId) {
    delete indexObj->index->Release(shapeId).release();
}

// polygonset key id s2textformat
void polygonSetCommand(client *c) {
    obj *tableObj;
    obj *key = c->argv[1];
    long long id;
    getLongLongFromObjectOrReply(c, c->argv[2], &id, "id is invalid");
    sds cdata = (sds)c->argv[3]->ptr;
    serverLog(LL_WARNING, "polygonset: key=%s, id=%llu, data=%s", (char *)key->ptr, id, cdata);
    string data = (char *)cdata;

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
    auto indexObj = (s2polygonIndex *) tableObj->ptr;
    auto mapIter = indexObj->map.find(id);
    if (mapIter == indexObj->map.end()) {
        serverLog(LL_WARNING, "polygon#%llu 不存在", id);
        // 未存在 新增
        int newShapeId = polygonAdd(indexObj, polygon.release());
        serverLog(LL_WARNING, "polygon#%llu 的shape id=%d", id, newShapeId);
        indexObj->map[id] = newShapeId;
        incrRefCount(tableObj);
    } else {
        serverLog(LL_WARNING, "polygon#%llu 已存在", id);
        // 已存在 插入新的 删除老的
        int oldShapeId = mapIter->second;
        serverLog(LL_WARNING, "polygon#%llu 的旧的shape id=%d", id, oldShapeId);
        int newShapeId = polygonAdd(indexObj, polygon.release());
        serverLog(LL_WARNING, "polygon#%llu 的新的shape id=%d", id, newShapeId);
        polygonDel(indexObj, polygon.release(), oldShapeId);
        indexObj->map[id] = newShapeId;
    }
    addReply(c, shared.ok);

    server.dirty++;
}


