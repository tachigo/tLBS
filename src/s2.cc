//
// Created by liuliwu on 2020-05-13.
//

#include "s2.h"
#include "zmalloc.h"

obj *createPolygonObject() {
    obj *o;
    auto *indexObj = new s2polygonIndex();
    indexObj->index = new MutableS2ShapeIndex();
    o = createObject(OBJ_TYPE_POLYGONS, indexObj);
    o->encoding = OBJ_ENCODING_S2INDEX;
    return o;
}
