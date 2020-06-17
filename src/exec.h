//
// Created by liuliwu on 2020-06-17.
//

#ifndef TLBS_EXEC_H
#define TLBS_EXEC_H

namespace tLBS {

    class Exec {
    public:
        virtual void setNeedClusterBroadcast(bool needClusterBroadcast) = 0;
        virtual bool getNeedClusterBroadcast() = 0;
    };
}

#endif //TLBS_EXEC_H
