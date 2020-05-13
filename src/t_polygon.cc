//
// Created by liuliwu on 2020-05-13.
//

#include "server.h"
#include "t_polygon.h"
#include "s2.h"
#include <s2/s2polygon.h>
#include <s2/s2text_format.h>

using namespace std;

// polygonset key id s2textformat
void polygonSetCommand(client *c) {
    obj *tableObj;
    obj *key = c->argv[1];
    sds id = (sds)c->argv[2]->ptr;
    sds cdata = (sds)c->argv[3]->ptr;
    serverLog(LL_WARNING, "polygonset: key=%s, id=%s, data=%s", (char *)key->ptr, id, cdata);
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

    dictEntry *de;
    de = dictFind(indexObj->dict, id);
    if (de == nullptr) {
        serverLog(LL_WARNING, "polygon#%s 不存在", id);
        // 未存在 新增
        int shapeId = indexObj->index->Add(absl::make_unique<S2Polygon::Shape>(polygon.release()));
        serverLog(LL_WARNING, "polygon#%s 的shape id=%d", id, shapeId);
        dictAdd(indexObj->dict, id, &shapeId);
        incrRefCount(tableObj);
    } else {
        serverLog(LL_WARNING, "polygon#%s 已存在", id);
        // 已存在 删除老的 更新
        int shapeId = indexObj->index->Add(absl::make_unique<S2Polygon::Shape>(polygon.release()));
        serverLog(LL_WARNING, "polygon#%s 的shape id=%d", id, shapeId);
        int oldShapeId = *(int *)dictGetVal(de);
        serverLog(LL_WARNING, "polygon#%s 的旧的shape id=%d", id, oldShapeId);
        delete indexObj->index->Release(oldShapeId).release();
        dictReplace(indexObj->dict, id, &shapeId);
    }
    addReply(c, shared.ok);

    server.dirty++;
}