//
// Created by liuliwu on 2020-05-11.
//

#ifndef TLBS_LOCALTIME_H
#define TLBS_LOCALTIME_H


static int is_leap_year(time_t year);
void nolocks_localtime(struct tm *tmp, time_t t, time_t tz, int dst);

#endif //TLBS_LOCALTIME_H
