//
// Created by liuliwu on 2020-06-03.
//

#include "json.h"

#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

using namespace tLBS;

Json::Json(std::string tpl) {
    this->doc = rapidjson::Document();
    this->doc.Parse(tpl.c_str());
    this->raw = "";
    this->pretty = "";
}

rapidjson::GenericValue<rapidjson::UTF8<char> >& Json::value() {
    return this->doc;
}

Json::~Json() {
}

rapidjson::Value& Json::get(const char *key) {
    return this->doc[key];
}

std::string Json::toString() {
    if (this->raw.size() == 0) {
        rapidjson::StringBuffer buf;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buf);
        this->doc.Accept(writer);
        this->raw = std::string(buf.GetString());
    }
    return this->raw;
}

std::string Json::toPretty() {
    if (this->pretty.size() == 0) {
        rapidjson::StringBuffer buf;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buf);
        this->doc.Accept(writer);
        this->pretty = std::string(buf.GetString());
    }
    return this->pretty;
}

Json* Json::createSuccessNumberJsonObj() {
    return new Json(R"({"errno": 0, "data": 0})");
}

Json* Json::createSuccessStringJsonObj() {
    return new Json(R"({"errno": 0, "data": ""})");
}

Json* Json::createSuccessBooleanJsonObj() {
    return new Json(R"({"errno": 0, "data": false})");
}

Json* Json::createSuccessArrayJsonObj() {
    return new Json(R"({"errno": 0, "data": []})");
}

Json* Json::createSuccessObjectJsonObj() {
    return new Json(R"({"errno": 0, "data": {}})");
}

Json* Json::createErrorJsonObj() {
    return new Json(R"({"errno": 0, "data": ""})");
}

Json* Json::createErrorJsonObj(int errorNo, const char *errorMsg) {
    Json *json = createErrorJsonObj();
    json->get("errno").SetInt(errorNo);
    json->get("data").SetString(rapidjson::GenericStringRef<char>(errorMsg));
    return json;
}

Json *Json::createSuccessStringJsonObj(const char *data) {
    Json *json = createSuccessStringJsonObj();
    json->get("data").SetString(rapidjson::GenericStringRef<char>(data));
    return json;
}


rapidjson::GenericStringRef<char> Json::createString(const char *str) {
    return rapidjson::GenericStringRef<char>(str);
}

rapidjson::GenericStringRef<char> Json::createString(std::string str) {
    return rapidjson::GenericStringRef<char>(str.c_str());
}


rapidjson::Document::AllocatorType& Json::getAllocator() {
    return this->doc.GetAllocator();
}