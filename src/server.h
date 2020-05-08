//
// Created by liuliwu on 2020-05-08.
//

#ifndef TLBS_SERVER_H
#define TLBS_SERVER_H

#include <cstdio>
#include <cstdlib>


#include "dict.h"


/* Error codes */
#define C_OK                    0
#define C_ERR                   -1

struct tLbsServer;
struct tLbsDb;
struct tLbsClient;
struct tLbsCommand;
struct tLbsObject;

typedef struct tLbsServer {
    pid_t pid;
    const char * pidFile;
    const char * configFile;
    tLbsDb * db;
    dict * commands; // 命令表
    int port;
} server;

typedef struct tLbsDb {

} db;

typedef struct tLbsClient {

} client;




/*-----------------------------------------------------------------------------
 * Data types 数据类型：点、线、多边形
 *----------------------------------------------------------------------------*/

#define OBJ_POINTS 0 /* (multi-)point 点 */
#define OBJ_LINES 1 /* (multi-)linestring 线 */
#define OBJ_POLYGONS 2 /* (multi-)polygon 多边形 */

#define OBJ_FORMAT_DEGREE 0
#define OBJ_FORMAT_RADIAN 1
#define OBJ_FORMAT_CELL_ID 2

typedef struct tLbsObject {
    unsigned type: 4;
    unsigned format: 4;
    int refCount;
    void * ptr;
} tobj;

const char * getObjectTypeName(tobj * o);

const char * getObjectTypeName(tobj * o) {
    const char * type;
    if (o == nullptr) {
        type = "none";
    }
    else {
        switch (o->type) {
            case OBJ_POINTS: type = "points"; break;
            case OBJ_LINES: type = "lines"; break;
            case OBJ_POLYGONS: type = "polygons"; break;
            default: type = "unknown"; break;
        }
    }
    return type;
}
/*--------------------------------------------------------
 * 前缀 point/line/polygon表示要操作的索引的类型
 *--------------------------------------------------------*/
// Point commands
void pointSetCommand(client * c);
void pointGetCommand(client * c);
void pointNearbyCommand(client * c);


// Line commands
void lineSetCommand(client * c);
void lineGetCommand(client * c);
void lineNearbyCommand(client * c);


// Polygon commands
void polygonSetCommand(client * c);
void polygonGetCommand(client * c);
void polygonNearbyCommand(client * c);
void polygonWithinCommand(client * c);

#endif //TLBS_SERVER_H
