//
// Created by 刘立悟 on 2020/5/18.
//

#ifndef TLBS_OBJECT_H
#define TLBS_OBJECT_H

#include <string>

namespace tLBS {
    class Object {
    private:
        unsigned type;
        unsigned encoding;
        unsigned long long refcount;
        void *ptr;

    public:
        Object();
        ~Object();
        unsigned getType();
        unsigned getEncoding();
        unsigned long long getRefCount();
        void incrRefCount();
        void decrRefCount();
        void resetRefCount();
        std::string getTypeName();
    };
}


#endif //TLBS_OBJECT_H
