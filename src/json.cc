//
// Created by liuliwu on 2020-06-03.
//

#include "json.h"
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

using namespace tLBS;

Json::Json(std::string tpl) {
    this->doc = new rapidjson::Document();
    this->doc->Parse(tpl.c_str());
}

Json::~Json() {
    delete this->doc;
}

rapidjson::Value* Json::get(std::string key) {
    return &(*this->doc)[key.c_str()];
}

std::string Json::toString() {
    rapidjson::StringBuffer buf;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buf);
    this->doc->Accept(writer);
    return buf.GetString();
}

Json* Json::createCmdSuccessNumberJsonObj() {
    return new Json(R"({"errno": 0, "data": 0})");
}

Json* Json::createCmdSuccessStringJsonObj() {
    return new Json(R"({"errno": 0, "data": ""})");
}

Json* Json::createCmdSuccessBooleanJsonObj() {
    return new Json(R"({"errno": 0, "data": false})");
}

Json* Json::createCmdSuccessArrayJsonObj() {
    return new Json(R"({"errno": 0, "data": []})");
}

Json* Json::createCmdSuccessObjectJsonObj() {
    return new Json(R"({"errno": 0, "data": {}})");
}

Json* Json::createCmdErrorJsonObj() {
    return new Json(R"({"errno": 0, "data": ""})");
}

Json* Json::createCmdErrorJsonObj(int errorNo, const char *errorMsg) {
    Json *json = createCmdErrorJsonObj();
    json->get("errno")->SetInt(errorNo);
    json->get("data")->SetString(rapidjson::GenericStringRef<char>(errorMsg));
    return json;
}

Json *Json::createCmdSuccessStringJsonObj(std::string data) {
    Json *json = createCmdSuccessStringJsonObj();
    json->get("data")->SetString(rapidjson::GenericStringRef<char>(data.c_str()));
    return json;
}