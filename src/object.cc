//
// Created by 刘立悟 on 2020/5/18.
//

#include "object.h"
#include "log.h"

using namespace tLBS;


Object::Object(unsigned int type, unsigned int encoding, void *data) {
    this->type = type;
    this->encoding = encoding;
    this->data = data;
    char buf[100];
    snprintf(buf, sizeof(buf), "obj[T:%s][E:%s]",
            this->getTypeName().c_str(), this->getEncodingName().c_str());
    this->info = buf;
    this->refcount = 1; // 默认是1
}

void* Object::getData() {
    return this->data;
}

Object::~Object() {
    info("销毁") << this->getInfo();
}

std::string Object::getInfo() {
    return this->info;
}

unsigned int Object::getType() {
    return this->type;
}

unsigned int Object::getEncoding() {
    return this->encoding;
}


unsigned long long Object::getRefCount() {
    return this->refcount;
}

void Object::incrRefCount() {
    this->refcount++;
}

void Object::decrRefCount() {
    this->refcount--;
}

void Object::resetRefCount() {
    this->refcount = 0;
}

std::string Object::getTypeName() {
    switch (this->type) {
        case ObjectType::OBJ_TYPE_GEO_POLYGON: return "geo_polygon";
        case ObjectType::OBJ_TYPE_GEO_LINESTRING: return "geo_linestring";
        case ObjectType::OBJ_TYPE_GEO_POINT: return "geo_point";
        case ObjectType::OBJ_TYPE_GEO_SHAPE: return "geo_shape";
        default: return "unknown";
    }
}

std::string Object::getEncodingName() {
    switch (this->encoding) {
        case ObjectEncoding::OBJ_ENCODING_S2GEOMETRY: return "s2geometry";
        default: return "unknown";
    }
}

Object* Object::createObject(unsigned int type, unsigned int encoding, void *data) {
    return new Object(type, encoding, data);
}

Object* Object::createGeoPolygonObject(void *data) {
    return createS2GeoPolygonObject(data);
}

Object* Object::createS2GeoPolygonObject(void *data) {
    return createObject(
            ObjectType::OBJ_TYPE_GEO_POLYGON,
            ObjectEncoding::OBJ_ENCODING_S2GEOMETRY,
            data
            );
}