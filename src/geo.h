//
// Created by liuliwu on 2020-05-14.
//

#ifndef TLBS_GEO_H
#define TLBS_GEO_H

#include <s2/mutable_s2shape_index.h>
#include "object.h"
#include <map>


class geoPolygonIndex {
public:
    std::map<string, int> map;
    std::map<int, string> data;
    MutableS2ShapeIndex *index;

    geoPolygonIndex();
    ~geoPolygonIndex();
};

class geoLineIndex {

};


class geoPointIndex {

};







//obj *createLineObject();
//obj *createPointObject();

#endif //TLBS_GEO_H
