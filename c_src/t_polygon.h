//
// Created by liuliwu on 2020-05-13.
//

#ifndef TLBS_T_POLYGON_H
#define TLBS_T_POLYGON_H

#include "client.h"

void polygonSetCommand(client *c);
void polygonDelCommand(client *c);
void polygonGetCommand(client *c);

obj *createPolygonObject();
void freePolygonObject(obj *o);

#define PERSISTENCE_SHARD_NUM 2
int persistenceLoadPolygonIndex(char *persistenceDataDir, int dbnum, obj *key, obj *tableObj);
int persistenceDumpPolygonIndex(char *persistenceDataDir, int dbnum, obj *key, obj *tableObj);

#endif //TLBS_T_POLYGON_H
