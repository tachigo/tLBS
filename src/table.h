//
// Created by 刘立悟 on 2020/6/4.
//

#ifndef TLBS_TABLE_H
#define TLBS_TABLE_H

#include "object.h"

namespace tLBS {

    class Table;

    class Db;

    typedef int (*tableSaverHandler)(std::string dataRootPath, std::string table, int shards, void *ptr);
    typedef int (*tableLoaderHandler)(std::string dataRootPath, std::string table, int shards, void *ptr);

    class Table : public Object {
    private:
        int db;
        std::string name;
        tableSaverHandler saverHandler;
        tableLoaderHandler loaderHandler;
        int shards;
        int version;

        int dirty;
        bool saving;
        time_t lastSave;
        bool loading;


    public:
        Table(int db, std::string name, unsigned int type, unsigned int encoding, void *data);
        ~Table();
        void setDb(int db);
        int getDb();
        std::string getName();
        std::string getMetadata();
        static Table *parseMetadata(std::string metadata);

        void setVersion(int version);
        int getVersion();
        void setShards(int shards);
        int getShards();

        void incrDirty(int incr);
        void decrDirty(int decr);
        void resetDirty();
        int getDirty();

        time_t getLastSave();
        void setSaving(bool saving);
        bool isSaving();
        void setLoading(bool loading);
        bool isLoading();

        void setSaverHandler(tableSaverHandler dumper);
        int callSaverHandler(std::string dataRootPath);
        void setLoaderHandler(tableLoaderHandler loader);
        int callLoaderHandler(std::string dataRootPath);

        static Table *createTable(int db, std::string name, unsigned int type, unsigned int encoding, void *data);
        static Table *createS2GeoPolygonTable(int db, std::string name);
    };
}

#endif //TLBS_TABLE_H
