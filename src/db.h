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
    private:
        int id;
        std::map<std::string, Object *> table;
        static std::vector<Db *> dbs;
        int dirty;
    public:
        Db(int id);
        ~Db();
        int getId();
        Object *lookupKey(std::string key, int flags);
        Object *lookupKeyRead(std::string key);
        Object *lookupKeyWrite(std::string key);
        Object *lookupKeyReadWithFlags(std::string key, int flags);
        Object *lookupKeyWriteWithFlags(std::string key, int flags);
        void tableAdd(std::string key, Object *data);
        bool tableExists(std::string key);
        int tableRemove(std::string key);

        void incrDirty(int incr);
        void decrDirty(int decr);
        void resetDirty();
        int getDirty();

        static Db* getDb(int id);
        static void init();
        static void free();
        static void save();
        // command
        static int dbSelect(Client *client);
        static int db(Client *client);
    };
}

#endif //TLBS_DB_H
