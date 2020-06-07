//
// Created by liuliwu on 2020-05-28.
//

#ifndef TLBS_LOG_H
#define TLBS_LOG_H

#include <glog/logging.h>

#define info(x) LOG(INFO) << x
#define warning(x) LOG(WARNING) << x
#define error(x) LOG(ERROR) << x
#define fatal(x) LOG(FATAL) << x

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
