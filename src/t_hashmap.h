//
// Created by liuliwu on 2020-06-18.
//

#ifndef TLBS_T_HASHMAP_H
#define TLBS_T_HASHMAP_H

#include "table.h"
#include <map>
#include <string>


namespace tLBS {
    class Connection;

    class HashMap {

    public:
        class StringStringHashMap: public TableEncoding {
        private:
            std::map<std::string, std::string> map;
        public:
            StringStringHashMap();
            ~StringStringHashMap();

            int set(std::string key, std::string value);
            void del(std::string key);
            std::string get(std::string key);
            void clear();

            uint64_t getSize();

            std::string getTmpFile(std::string table, int shard);
            std::string getDatFile(std::string table, int shard);
            int dump(std::string dataRootPath, std::string table, int shards);
            int load(std::string dataRootPath, std::string table, int shards);
            int send(std::string dataRootPath, std::string table, int shards, std::string prefix, Connection *conn);
            int receive(std::string line);
            // dumper handler
            static int dumper(std::string dataRootPath, std::string table, int shards, void *ptr);
            // loader handler
            static int loader(std::string dataRootPath, std::string table, int shards, void *ptr);
            // sender handler
            static int sender(std::string dataRootPath, std::string table, int shards, void *ptr, std::string prefix, Connection *conn);
            // receiver handler
            static int receiver(void *ptr, std::string line);
        };


        // exec
        // sshmset table key value
        static int execSSHashMapSet(Exec *exec, Connection *conn, std::vector<std::string> args);
        // sshmget table key
        static int execSSHashMapGet(Exec *exec, Connection *conn, std::vector<std::string> args);
        // sshmdel table key
        static int execSSHashMapDel(Exec *exec, Connection *conn, std::vector<std::string> args);
        // sshmclear table
        static int execSSHashMapClear(Exec *exec, Connection *conn, std::vector<std::string> args);
    };
}

#endif //TLBS_T_HASHMAP_H
