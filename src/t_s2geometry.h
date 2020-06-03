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
        };

        // command
        static int test(Client *client);

        // s2polyset table id data
        static int cmdSetPolygon(Client *client);

        // s2polyget table id
        static int cmdGetPolygon(Client *client);

        // s2polydel table id
        static int cmdDelPolygon(Client *client);
    };
}


#endif //TLBS_T_S2GEOMETRY_H
