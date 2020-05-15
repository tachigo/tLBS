//
// Created by liuliwu on 2020-05-14.
//

#include "geo.h"


geoPolygonIndex::geoPolygonIndex() {
    this->index = new MutableS2ShapeIndex();
}

geoPolygonIndex::~geoPolygonIndex() {
    this->index->Clear();
}
