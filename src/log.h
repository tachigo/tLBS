//
// Created by liuliwu on 2020-05-28.
//

#ifndef TLBS_LOG_H
#define TLBS_LOG_H

#include <glog/logging.h>
#include "config.h"

#define info(x) (LOG(INFO) << CUR_SERVER << x)
#define warning(x) (LOG(WARNING) << CUR_SERVER << x)
#define error(x) (LOG(ERROR) << CUR_SERVER << x)
#define fatal(x) (LOG(FATAL) << CUR_SERVER << x)

namespace tLBS {
    class Log {

    private:
    public:
        Log() = default;
        ~Log() = delete;
        static void init(const char *programName);
        static void free();
    };
}

#endif //TLBS_LOG_H
