//
// Created by liuliwu on 2020-05-14.
//

#ifndef TLBS_CHILDINFO_H
#define TLBS_CHILDINFO_H

#define CHILD_INFO_MAGIC 0xC17DDA7A12345678LL
//#define CHILD_INFO_TYPE_RDB 0
#define CHILD_INFO_TYPE_PERSISTENCE 0
#define CHILD_INFO_TYPE_AOF 1
#define CHILD_INFO_TYPE_MODULE 3

void openChildInfoPipe();
void closeChildInfoPipe();
void sendChildCOWInfo(int ptype, const char *pname);
void sendChildInfo(int ptype);
void receiveChildInfo();

#endif //TLBS_CHILDINFO_H
