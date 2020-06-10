//
// Created by 刘立悟 on 2020/5/18.
//

#ifndef TLBS_DB_H
#define TLBS_DB_H

#include <string>
#include <map>
#include <vector>
#include "connection.h"

#define DB_FLAGS_LOOKUP_NONE 0

namespace tLBS {

    class Client;

    class Table;

    class Db {
    public:
        class SaveParam {
        private:
            time_t seconds;
            int changes;
        public:
            SaveParam(time_t seconds, int changes);
            ~SaveParam();
            time_t getSeconds();
            int getChanges();
        };
    private:
        int id;
        std::map<std::string, Table *> tables;
        static std::vector<Db *> dbs;
        int dirty;
        time_t lastSave;
        std::vector<SaveParam *> saveParams;
        std::string info;
        bool saving;
        bool loading;
    public:
        Db(int id);
        ~Db();
        int getId();
        std::string getInfo();
        std::string getDataPath();
        Table *lookupTable(std::string key, int flags);
        Table *lookupTableRead(std::string key);
        Table *lookupTableWrite(std::string key);
        Table *lookupTableReadWithFlags(std::string key, int flags);
        Table *lookupTableWriteWithFlags(std::string key, int flags);
        void tableAdd(std::string key, Table *table);
        bool tableExists(std::string key);
        void tableRemove(std::string key);

        std::string getTmpFile();
        std::string getDatFile();
        void load();
        void save();
        static void saveAll();
        static void loadAll();

        void setSaving(bool saving);
        bool isSaving();
        void setLoading(bool loading);
        bool isLoading();
        void resetSaveParams();
        void appendSaveParam(time_t seconds, int changes);
        std::vector<SaveParam *> getSaveParams();
        time_t getLastSave();

        void incrDirty(int incr);
        void decrDirty(int decr);
        void resetDirty();
        int getDirty();

        static Db* getDb(int id);
        static void init();
        static void free();

        static void *threadProcess(void *arg);

        // command
        static int execDb(Connection *conn, std::vector<std::string> args);
    };
}

#endif //TLBS_DB_H
