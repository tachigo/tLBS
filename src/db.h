//
// Created by 刘立悟 on 2020/5/18.
//

#ifndef TLBS_DB_H
#define TLBS_DB_H

#include <string>
#include <map>
#include <vector>

#define DB_FLAGS_LOOKUP_NONE 0

namespace tLBS {

    class Client;

    class Object;

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
        std::map<std::string, Object *> table;
        static std::vector<Db *> dbs;
        int dirty;
        time_t lastSave;
        std::vector<SaveParam *> saveParams;
        std::string info;
    public:
        Db(int id);
        ~Db();
        int getId();
        std::string getInfo();
        std::string getDataPath();
        Object *lookupKey(std::string key, int flags);
        Object *lookupKeyRead(std::string key);
        Object *lookupKeyWrite(std::string key);
        Object *lookupKeyReadWithFlags(std::string key, int flags);
        Object *lookupKeyWriteWithFlags(std::string key, int flags);
        void tableAdd(std::string key, Object *data);
        bool tableExists(std::string key);
        void tableRemove(std::string key);

        std::string getTmpFile();
        std::string getDatFile();
        void save();
        static void saveAll();

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
        static void cron(long long id, void *data);

        // command
        static int cmdDb(Client *client);
    };
}

#endif //TLBS_DB_H
