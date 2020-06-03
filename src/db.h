//
// Created by 刘立悟 on 2020/5/18.
//

#ifndef TLBS_DB_H
#define TLBS_DB_H

#include <string>
#include <map>
#include <vector>

namespace tLBS {

    class Client;

    class Db {
    private:
        int id;
        std::map<std::string, void *> table;
        static std::vector<Db *> dbs;
    public:
        Db(int id);
        ~Db();
        int getId();
        void *lookupKey(std::string key, int flags);
        void *lookupKeyRead(std::string key);
        void *lookupKeyWrite(std::string key);
        void *lookupKeyReadWithFlags(std::string key, int flags);
        void *lookupKeyWriteWithFlags(std::string key, int flags);
        void tableAdd(std::string key, void *data);
        int tableExists(std::string key);
        int tableRemove(std::string key);


        static void init();
        static void free();
        // command
        static void dbSelect(Client *client);
    };
}

#endif //TLBS_DB_H
