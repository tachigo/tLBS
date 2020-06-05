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
    class Client;

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
            // dumper handler
            static int dumper(std::string dataRootPath, std::string table, int shards, void *ptr);
            // loader handler
            static int loader(std::string dataRootPath, std::string table, int shards, void *ptr);
            // mover
        };

        // command
        static int execTest(Client *client);

        // s2polyset table id data
        static int execSetPolygon(Client *client);

        // s2polyget table id
        static int execGetPolygon(Client *client);

        // s2polydel table id
        static int execDelPolygon(Client *client);
    };
}


#endif //TLBS_T_S2GEOMETRY_H
