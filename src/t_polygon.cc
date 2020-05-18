//
// Created by liuliwu on 2020-05-13.
//

#include "server.h"
#include "t_polygon.h"
#include "t_string.h"
#include "geo.h"
#include "debug.h"
#include "dict.h"
#include "zmalloc.h"
#include <s2/s2polygon.h>
#include <s2/s2text_format.h>
#include <unistd.h>
#include <cstdio>
#include <iostream>
#include <fstream>

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
    indexObj->index->ForceBuild();
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
    tableObj = lookupKeyRead(c->db,key);
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
    } else {
        serverLog(LL_WARNING, "polygon#%s 已存在", id);
        // 已存在 插入新的 删除老的
        int oldShapeId = mapIter->second;
        indexObj->data[oldShapeId].erase();
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

obj *createPolygonObject() {
    obj *o;
    auto *indexObj = new geoPolygonIndex();
    o = createObject(OBJ_TYPE_POLYGONS, indexObj);
    o->encoding = OBJ_ENCODING_S2INDEX;
    return o;
}

void freePolygonObject(obj *o) {
    switch (o->encoding) {
        case OBJ_ENCODING_S2INDEX:
            delete (geoPolygonIndex *)o->ptr;
            break;
        default:
            serverPanic("Unknown polygon encoding type");
    }
}

const char *getPersistenceTempFile(int dbnum, obj *key, int shardid) {
    char tmpfile[256];
    snprintf(tmpfile,sizeof(tmpfile),"db#%d/polygon<%s>%d_%d.tmp", dbnum, (const char *)key->ptr, shardid, (int) getpid());
    return sdsnew(tmpfile);
}

const char *getPersistenceFile(int dbnum, obj *key, int shardid) {
    char filename[256];
    snprintf(filename,sizeof(filename),"db#%d/polygon<%s>%d.dat", dbnum, (const char *)key->ptr, shardid);
    return sdsnew(filename);
}

int persistenceLoadPolygonIndex(char *persistenceDataDir, int dbnum, obj *key, obj *tableObj) {
    auto indexObj = (geoPolygonIndex *)tableObj->ptr;
    int shardnum = PERSISTENCE_SHARD_NUM;
    for (int i = 0; i < shardnum; i++) {
        // 对数据进行遍历
        const char *filename = sdscat(sdsdup(persistenceDataDir), getPersistenceFile(dbnum, key, i));
        std::ifstream in(filename);
        if (!in) {
            serverLog(LL_WARNING,
                      "Failed opening the persistence file %s "
                      "for saving: %s",
                      filename,
                      strerror(errno));
            serverPanic("无法打开polygon数据文件");
        }
        unique_ptr<S2Polygon> polygon;
        S2Debug debug_override;
        std::string line;
        while (getline(in, line)) {
            int *count = (int *)zmalloc(sizeof(int *));
            sds *arr = sdssplitlen(line.c_str(), line.size(), "+", 1, count);
            if (*count != 2) {
                serverPanic("polygon数据格式错误");
            }
            string id = arr[0];
            string data = arr[1];
            if (!s2textformat::MakePolygon(data, &polygon, debug_override)) {
                serverPanic("Failed to transform to s2 polygon format.");
            }
            int newShapeId = polygonAdd(indexObj, polygon.release());
            indexObj->map[id] = newShapeId;
            indexObj->data[newShapeId] = data;
            zfree(count);
        }
        in.close();
    }
    return C_OK;
}

int persistenceDumpPolygonIndex(char *persistenceDataDir, int dbnum, obj *key, obj *tableObj) {
    auto indexObj = (geoPolygonIndex *)tableObj->ptr;
    int shardnum = PERSISTENCE_SHARD_NUM;
    FILE *fps[shardnum];
    for (int i = 0; i < shardnum; i++) {
        // 对数据进行遍历
        FILE *fp;
        const char *tmpfile = sdscat(sdsdup(persistenceDataDir), getPersistenceTempFile(dbnum, key, i));
//        serverLog(LL_WARNING, "持久化文件=%s", tmpfile);
        fp = fopen(tmpfile,"w");
        if (!fp) {
            serverLog(LL_WARNING,
                      "Failed opening the persistence file %s "
                      "for saving: %s",
                      tmpfile,
                      strerror(errno));
            goto werr;
        }
        fps[i] = fp;
    }
    for (auto iter = indexObj->map.begin(); iter != indexObj->map.end(); iter++) {
        string id = iter->first;
        int shapeId = iter->second;
        string data = indexObj->data.find(shapeId)->second;
//        serverLog(LL_WARNING, "%s+%s", id.c_str(), data.c_str());
        uint64_t hash = dictGenHashFunction(id.c_str(), id.size());
        int shard = hash % shardnum;
//        serverLog(LL_WARNING, "%s shard %d", id.c_str(), shard);
        FILE *fp = fps[shard];
        string line = id + "+" + data + "\n";
        fputs(line.c_str(), fp);
    }

    for (int i = 0; i < shardnum; i++) {
        FILE *fp = fps[i];
        if (fp != nullptr) {
            /* Make sure data will not remain on the OS's output buffers */
            if (fflush(fp) == EOF) goto werr;
            if (fsync(fileno(fp)) == -1) goto werr;
            if (fclose(fp) == EOF) goto werr;
        }
    }

    for (int i = 0; i < shardnum; i++) {
        const char *filename = sdscat(sdsdup(persistenceDataDir), getPersistenceFile(dbnum, key, i));
        const char *tmpfile = sdscat(sdsdup(persistenceDataDir), getPersistenceTempFile(dbnum, key, i));
        /* Use RENAME to make sure the DB file is changed atomically only
         * if the generate DB file is ok. */
        if (rename(tmpfile,filename) == -1) {
            serverLog(LL_WARNING,
                      "Error moving temp Data file %s on the final "
                      "destination %s: %s",
                      tmpfile,
                      filename,
                      strerror(errno));
            unlink(tmpfile);
            return C_ERR;
        }
    }

    for (int i = 0; i < shardnum; i++) {
        const char *tmpfile = sdscat(sdsdup(persistenceDataDir), getPersistenceTempFile(dbnum, key, i));
        FILE *fp = fps[i];
        if (fp != nullptr) {
            fclose(fp);
            unlink(tmpfile);
        }
    }
    return C_OK;

werr:
    for (int i = 0; i < shardnum; i++) {
        const char *tmpfile = sdscat(sdsdup(persistenceDataDir), getPersistenceTempFile(dbnum, key, i));
        FILE *fp = fps[i];
        if (fp != nullptr) {
            fclose(fp);
            unlink(tmpfile);
        }
    }
    serverLog(LL_WARNING,"Write error saving DB on disk: %s", strerror(errno));
    return C_ERR;
}
