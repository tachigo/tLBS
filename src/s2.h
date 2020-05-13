//
// Created by liuliwu on 2020-05-13.
//

#ifndef TLBS_S2_H
#define TLBS_S2_H

#include <s2/mutable_s2shape_index.h>
#include "object.h"
#include <map>

using namespace std;

class s2polygonIndex {
public:
    map<long long, long long> map;
    MutableS2ShapeIndex *index;
};

obj *createPolygonObject();

#endif //TLBS_S2_H
