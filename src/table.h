//
// Created by 刘立悟 on 2020/6/4.
//

#ifndef TLBS_TABLE_H
#define TLBS_TABLE_H

#include "object.h"

namespace tLBS {

    class Table;

    class Db;

    typedef int (*tableDumperHandler)(std::string dataRootPath, std::string table, int shards, void *ptr);
    typedef int (*tableLoaderHandler)(std::string dataRootPath, std::string table, int shards, void *ptr);

    class Table : public Object {
    private:
        int db;
        std::string name;
        tableDumperHandler dumperHandler;
        tableLoaderHandler loaderHandler;
        int shards;

        // mover handler
    public:
        Table(int db, std::string name, unsigned int type, unsigned int encoding, void *data);
        ~Table();
        void setDb(int db);
        int getDb();
        std::string getName();
        std::string getMetadata();
        static Table *parseMetadata(std::string metadata);


        void setShards(int shards);
        int getShards();

        void setDumperHandler(tableDumperHandler dumper);
        int callDumperHandler(std::string dataRootPath);
        void setLoaderHandler(tableLoaderHandler loader);
        int callLoaderHandler(std::string dataRootPath);

        static Table *createTable(int db, std::string name, unsigned int type, unsigned int encoding, void *data);
        static Table *createS2GeoPolygonTable(int db, std::string name);
    };
}

#endif //TLBS_TABLE_H
