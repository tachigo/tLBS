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
        rapidjson::Document doc;
        std::string str;
    public:
        explicit Json(std::string tpl);
        ~Json();
        rapidjson::Value &get(const char *key);
        std::string toString();
        rapidjson::Document::AllocatorType& getAllocator();
        rapidjson::GenericValue<rapidjson::UTF8<char> >& value();


        static Json *createSuccessNumberJsonObj();
        static Json *createSuccessStringJsonObj();
        static Json *createSuccessBooleanJsonObj();
        static Json *createSuccessArrayJsonObj();
        static Json *createSuccessObjectJsonObj();

        static Json *createErrorJsonObj();
        static Json *createErrorJsonObj(int errorNo, const char *errorMsg);
        static Json *createSuccessStringJsonObj(const char *data);


        static rapidjson::GenericStringRef<char> createString(const char *str);
        static rapidjson::GenericStringRef<char> createString(std::string str);
    };
}

#endif //TLBS_JSON_H
