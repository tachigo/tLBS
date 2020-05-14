//
// Created by liuliwu on 2020-05-14.
//

#include "geo.h"
#include "common.h"

geoPolygonIndex::geoPolygonIndex() {
    this->index = new MutableS2ShapeIndex();
}

geoPolygonIndex::~geoPolygonIndex() {
    this->index->Clear();
}

int geoPolygonIndex::dumpPersistenceData() {

    return C_OK;
}

obj *createPolygonObject() {
    obj *o;
    auto *indexObj = new geoPolygonIndex();
    o = createObject(OBJ_TYPE_POLYGONS, indexObj);
    o->encoding = OBJ_ENCODING_S2INDEX;
    return o;
}