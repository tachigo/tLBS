//
// Created by 刘立悟 on 2020/5/18.
//

#ifndef TLBS_DB_H
#define TLBS_DB_H

#include <string>
#include <map>
#include "object.h"

namespace tLBS {
    class Db {
    private:
        int id;
        std::map<std::string, void *> table;

    public:
        Db();
        ~Db();
        int getId();
        Object *lookupKey(Object *keyObj, int flags);
        Object *lookupKeyRead(Object *keyObj);
        Object *lookupKeyWrite(Object *keyObj);
        Object *lookupKeyReadWithFlags(Object *keyObj, int flags);
        Object *lookupKeyWriteWithFlags(Object *keyObj, int flags);
        void tableAdd(Object *keyObj, Object *valObj);
        int tableExists(Object *keyObj);
        int tableRemove(Object *keyObj);
    };
}

#endif //TLBS_DB_H
