//
// Created by 刘立悟 on 2020/5/18.
//

#ifndef TLBS_OBJECT_H
#define TLBS_OBJECT_H

#include <string>

#define OBJ_FLAGS_NONE 0


namespace tLBS {

    typedef enum {
        OBJ_TYPE_GEO_POLYGON = (1<<0), // 多边形
        OBJ_TYPE_GEO_LINESTRING = (1<<1), // 线
        OBJ_TYPE_GEO_POINT = (1<<2), // 点
        OBJ_TYPE_GEO_SHAPE = (1<<3), // 形状
    } ObjectType;

    typedef enum {
        OBJ_ENCODING_S2GEOMETRY = (1<<0), // google s2 geometry
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
        void *getData();
        unsigned int getType();
        unsigned int getEncoding();
        std::string getTypeName();
        std::string getEncodingName();

        unsigned long long getRefCount();
        void incrRefCount();
        void decrRefCount();
        void resetRefCount();

        static void free(Object *obj);

        static Object *createObject(unsigned int type, unsigned int encoding, void *data);
        static Object *createS2GeoPolygonObject(void *data);
        static Object *createGeoPolygonObject(void *data);
    };
}


#endif //TLBS_OBJECT_H
