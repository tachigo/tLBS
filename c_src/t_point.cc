//
// Created by liuliwu on 2020-05-08.
//
#include "t_point.h"
#include "zmalloc.h"
#include <s2/s2cell_id.h>
#include <s2/s2latlng.h>

uint64 calcCellIdByLatLon(double lat, double lon) {
    S2LatLng latLng = S2LatLng::FromDegrees(lat, lon);
    S2CellId cellId = S2CellId(latLng);
    return cellId.id();
}


void pointTestCommand(client *c) {
    // pointtest key id lat lon
    obj *key = c->argv[1];
    sds id = (sds)c->argv[2]->ptr;
    auto *lat = (double *)zmalloc(sizeof(double));
    auto *lon = (double *)zmalloc(sizeof(double));
    getDoubleFromObjectOrReply(c, c->argv[3], lat, "lat is invalid");
    getDoubleFromObjectOrReply(c, c->argv[4], lon, "lon is invalid");
    serverLog(LL_WARNING, "args: key=%s, id=%s, lat=%f, lon=%f", (char *)key->ptr, id, *lat, *lon);
    uint64 cellid = calcCellIdByLatLon(*lat, *lon);
    serverLog(LL_WARNING, "cellid=%llu", cellid);
}