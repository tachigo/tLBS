//
// Created by liuliwu on 2020-06-02.
//

#ifndef TLBS_T_S2GEOMETRY_H
#define TLBS_T_S2GEOMETRY_H

#include <string>
#include <map>
#include <s2/mutable_s2shape_index.h>
#include <s2/s2polygon.h>


namespace tLBS {
    class Connection;

    class S2Geometry {

    public:

        class PolygonIndex {
        private:
            MutableS2ShapeIndex *index;
            std::map<std::string, int> id2shapeId;
            std::map<int, std::string> shapeId2Data;
        public:
            PolygonIndex();
            ~PolygonIndex();
            int addPolygon(std::string id, std::string data);
            void delPolygon(int shapeId);
            int findShapeIdById(std::string id);
            std::string findDataByShapeId(int shapeId);
            std::string findDataById(std::string id);
            void flush();

            std::string getTmpFile(std::string table, int shard);
            std::string getDatFile(std::string table, int shard);
            int dump(std::string dataRootPath, std::string table, int shards);
            int load(std::string dataRootPath, std::string table, int shards);
            int send(std::string dataRootPath, std::string table, int shards, int db, Connection *conn);
            int receive(std::string line);
            // dumper handler
            static int dumper(std::string dataRootPath, std::string table, int shards, void *ptr);
            // loader handler
            static int loader(std::string dataRootPath, std::string table, int shards, void *ptr);
            // sender handler
            static int sender(std::string dataRootPath, std::string table, int shards, void *ptr, int db, Connection *conn);
            // receiver handler
            static int receiver(void *ptr, std::string line);
        };

        // command
        static int execTest(Connection *conn, std::vector<std::string> args);

        // s2polyset table id data
        static int execSetPolygon(Connection *conn, std::vector<std::string> args);

        // s2polyget table id
        static int execGetPolygon(Connection *conn, std::vector<std::string> args);

        // s2polydel table id
        static int execDelPolygon(Connection *conn, std::vector<std::string> args);

        // s2forcebuild table
        static int execForceBuild(Connection *conn, std::vector<std::string> args);
    };
}


#endif //TLBS_T_S2GEOMETRY_H
