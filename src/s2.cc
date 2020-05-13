//
// Created by liuliwu on 2020-05-13.
//

#include "s2.h"
#include "zmalloc.h"
#include "common.h"

dictType polygonDictType = {
        dictSdsHash,               /* hash function */
        nullptr,                      /* key dup */
        nullptr,                      /* val dup */
        dictSdsKeyCompare,         /* key compare */
        nullptr,                      /* Note: SDS string shared & freed by skiplist */
        nullptr                       /* val destructor */
};

obj *createPolygonObject() {
    obj *o;
    auto *indexObj = (s2polygonIndex *)zmalloc(sizeof(s2polygonIndex));
    indexObj->dict = dictCreate(&polygonDictType, nullptr);
    indexObj->index = new MutableS2ShapeIndex();
    o = createObject(OBJ_TYPE_POLYGONS, indexObj);
    o->encoding = OBJ_ENCODING_S2INDEX;
    return o;
}
