//
// Created by 刘立悟 on 2020/5/18.
//

#ifndef TLBS_OBJECT_H
#define TLBS_OBJECT_H

#include <string>

#define OBJ_FLAGS_NONE 0


namespace tLBS {

    typedef enum {
        OBJ_TYPE_GEO_POLYGON = 1, // 多边形
        OBJ_TYPE_GEO_LINESTRING = 2, // 线
        OBJ_TYPE_GEO_POINT = 3, // 点
        OBJ_TYPE_GEO_SHAPE = 4, // 形状

        OBJ_TYPE_HASH_MAP = 5, // hashmap
    } ObjectType;

    typedef enum {
        OBJ_ENCODING_S2GEOMETRY = 1, // google s2 geometry
        OBJ_ENCODING_H3GEOMETRY = 2, // uber h3 geometry
        OBJ_ENCODING_STRING_STRING_HASH_MAP = 3, // string -> string hash map
    } ObjectEncoding;

    class Object {
    private:
        unsigned int type;
        unsigned int encoding;
        unsigned long long refcount;
        void *data;
        std::string info;

    public:
        Object(unsigned int type, unsigned int encoding, void *data);
        ~Object();
        std::string getInfo();
        void setInfo(std::string info);
        void *getData();
        unsigned int getType();
        unsigned int getEncoding();
        std::string getTypeName();
        std::string getEncodingName();

        unsigned long long getRefCount();
        void incrRefCount();
        void decrRefCount();
        void resetRefCount();

//        static void free(Object *obj);

        static Object *createObject(unsigned int type, unsigned int encoding, void *data);

    };
}


#endif //TLBS_OBJECT_H
