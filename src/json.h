//
// Created by liuliwu on 2020-06-03.
//

#ifndef TLBS_JSON_H
#define TLBS_JSON_H

#include <rapidjson/document.h>
#include <string>

namespace tLBS {
    class Json {
    private:
        rapidjson::Document* doc;
    public:
        Json(std::string tpl);
        ~Json();
        rapidjson::Value *get(std::string key);
        std::string toString();

        static Json *createCmdSuccessNumberJsonObj();
        static Json *createCmdSuccessStringJsonObj();
        static Json *createCmdSuccessBooleanJsonObj();
        static Json *createCmdSuccessArrayJsonObj();
        static Json *createCmdSuccessObjectJsonObj();

        static Json *createCmdErrorJsonObj();
        static Json *createCmdErrorJsonObj(int errorNo, const char *errorMsg);
        static Json *createCmdSuccessStringJsonObj(std::string data);
    };
}

#endif //TLBS_JSON_H
