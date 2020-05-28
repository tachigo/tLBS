//
// Created by liuliwu on 2020-05-28.
//

#ifndef TLBS_EL_H
#define TLBS_EL_H

#define EL_OK 0;
#define EL_ERR -1;

#define EL_NONE 0; // 没有注册的事件
#define EL_READABLE 1; // 当文件描述符可读的时候发送事件
#define EL_WRITABLE 2; // 当文件描述符可写的时候发送事件


namespace tLBS {
    class EventLoop {

    };
}

#endif //TLBS_EL_H
