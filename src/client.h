//
// Created by 刘立悟 on 2020/5/18.
//

#ifndef TLBS_CLIENT_H
#define TLBS_CLIENT_H

namespace tLBS {
    class Client {
    private:
        uint64_t id; // 客户端id
        Db *db; // 客户端连接的库
        Object *name; // 当前客户端的名称
        time_t ctime; // 客户端创建时间
        uint64_t flags; // 客户端标记

    public:
        Client();
        ~Client();
    };
}

#endif //TLBS_CLIENT_H
