//
// Created by liuliwu on 2020-05-08.
//

#ifndef TLBS_T_POINT_H
#define TLBS_T_POINT_H


#include <s2/base/integral_types.h>
#include "client.h"

uint64 calcCellIdByLatLon(double lat, double lon);

void pointTestCommand(client *c);

#endif //TLBS_T_POINT_H
