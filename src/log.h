//
// Created by liuliwu on 2020-05-28.
//

#ifndef TLBS_LOG_H
#define TLBS_LOG_H

#include <glog/logging.h>

#define info(x) LOG(INFO) << x
#define error(x) LOG(ERROR) << x
#define warning(x) LOG(WARNING) << x

namespace tLBS {
    class Log {

    private:
        explicit Log(const char *programName);
    public:
        ~Log();
        static void init(const char *programName);
    };
}

#endif //TLBS_LOG_H
