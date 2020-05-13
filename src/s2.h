//
// Created by liuliwu on 2020-05-13.
//

#ifndef TLBS_S2_H
#define TLBS_S2_H

#include <s2/mutable_s2shape_index.h>
#include "object.h"
#include "dict.h"

typedef struct s2polygonIndex {
    dict *dict;
    MutableS2ShapeIndex *index;
} s2polygonIndex;

obj *createPolygonObject();

#endif //TLBS_S2_H
