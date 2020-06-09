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
        std::string str;
    public:
        Json(std::string tpl);
        ~Json();
        rapidjson::Value *get(const char *key);
        std::string toString();

        static Json *createSuccessNumberJsonObj();
        static Json *createSuccessStringJsonObj();
        static Json *createSuccessBooleanJsonObj();
        static Json *createSuccessArrayJsonObj();
        static Json *createSuccessObjectJsonObj();

        static Json *createErrorJsonObj();
        static Json *createErrorJsonObj(int errorNo, const char *errorMsg);
        static Json *createSuccessStringJsonObj(const char *data);
    };
}

#endif //TLBS_JSON_H
