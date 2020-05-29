//
// Created by liuliwu on 2020-05-28.
//

#ifndef TLBS_LOG_H
#define TLBS_LOG_H

#include <glog/logging.h>

#define info(x) LOG(INFO) << "\t\t\t" << x
#define error(x) LOG(ERROR) << "\t\t\t" << x
#define warning(x) LOG(WARNING) << "\t\t\t" << x
#define fatal(x) LOG(FATAL) << "\t\t\t" << x

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
