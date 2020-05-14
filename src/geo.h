//
// Created by liuliwu on 2020-05-14.
//

#ifndef TLBS_GEO_H
#define TLBS_GEO_H

#include <s2/mutable_s2shape_index.h>
#include "object.h"
#include <map>

using namespace std;

class geoPolygonIndex {
public:
    ::map<string, int> map;
    ::map<int, string> data;
    MutableS2ShapeIndex *index;

    geoPolygonIndex();
    ~geoPolygonIndex();

    int dumpPersistenceData();
};

class geoLineIndex {

};


class geoPointIndex {

};






obj *createPolygonObject();
obj *createLineObject();
obj *createPointObject();

#endif //TLBS_GEO_H
