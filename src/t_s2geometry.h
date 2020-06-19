//
// Created by liuliwu on 2020-06-02.
//

#ifndef TLBS_T_S2GEOMETRY_H
#define TLBS_T_S2GEOMETRY_H

#include <string>
#include <map>
#include <vector>
#include <s2/mutable_s2shape_index.h>
#include <s2/s2polygon.h>
#include "exec.h"
#include "table.h"

namespace tLBS {
    class Connection;

    class S2Geometry {

    public:

        class PolygonIndex: public TableEncoding {
        private:
            MutableS2ShapeIndex *index;
            std::map<std::string, int> id2shapeId;
            std::map<int, std::string> shapeId2id;
            std::map<int, std::string> shapeId2Data;
        public:
            PolygonIndex();
            ~PolygonIndex();
            int addPolygon(std::string id, std::string data);
            void delPolygon(int shapeId);
            // gps定位
            int locatePolygon(double lat, double lon, std::vector<std::string> *ret);
            // 最近的
            int closestPolygon(double lat, double lon, double distance, std::vector<std::map<std::string, std::string>> *ret);
            // 点半径范围内的
            int nearbyPolygon(double lat, double lon, double distance, std::vector<std::map<std::string, std::string>> *ret);

            std::string findIdByShapeId(int shapeId);
            int findShapeIdById(std::string id);
            std::string findDataByShapeId(int shapeId);
            std::string findDataById(std::string id);
            void flush();

            uint64_t getSize();

            std::string getTmpFile(std::string table, int shard);
            std::string getDatFile(std::string table, int shard);
            int dump(std::string dataRootPath, std::string table, int shards);
            int load(std::string dataRootPath, std::string table, int shards);
            int send(std::string dataRootPath, std::string table, int shards, std::string prefix, Connection *conn);
            int receive(std::string line);
            // dumper handler
            static int dumper(std::string dataRootPath, std::string table, int shards, void *ptr);
            // loader handler
            static int loader(std::string dataRootPath, std::string table, int shards, void *ptr);
            // sender handler
            static int sender(std::string dataRootPath, std::string table, int shards, void *ptr, std::string prefix, Connection *conn);
            // receiver handler
            static int receiver(void *ptr, std::string line);
        };

        // exec

        // s2polyset table id data
        static int execPolygonSet(Exec *exec, Connection *conn, std::vector<std::string> args);

        // s2polyget table id
        static int execPolygonGet(Exec *exec, Connection *conn, std::vector<std::string> args);

        // s2polydel table id
        static int execPolygonDel(Exec *exec, Connection *conn, std::vector<std::string> args);

        // s2build table
        static int execForceBuild(Exec *exec, Connection *conn, std::vector<std::string> args);

        // s2polyloc table lat lon
        static int execPolygonLocate(Exec *exec, Connection *conn, std::vector<std::string> args);

        // s2polyclosest table lat lon distance
        static int execPolygonClosest(Exec *exec, Connection *conn, std::vector<std::string> args);

        // s2polynearby table lat lon distance
        static int execPolygonNearby(Exec *exec, Connection *conn, std::vector<std::string> args);
    };
}


#endif //TLBS_T_S2GEOMETRY_H
