//
// Created by 刘立悟 on 2020/6/4.
//

#ifndef TLBS_TABLE_H
#define TLBS_TABLE_H

#include "object.h"

namespace tLBS {

    class Table;

    class Db;

    class Table : public Object {
    private:
        int db;
        std::string name;
        // dumper

        // loader

        // mover
    public:
        Table(int db, std::string name, unsigned int type, unsigned int encoding, void *data);
        ~Table();
        void setDb(int db);
        int getDb();
        std::string getName();

        static Table *createTable(int db, std::string name, unsigned int type, unsigned int encoding, void *data);
        static Table *createS2GeoPolygonTable(int db, std::string name, void *data);
    };
}

#endif //TLBS_TABLE_H
