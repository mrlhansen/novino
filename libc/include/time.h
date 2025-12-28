#ifndef TIME_H
#define TIME_H

#ifndef NULL
#define NULL ((void*)0)
#endif

typedef long time_t;

struct tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};

char *asctime(const struct tm *tm);
time_t mktime(struct tm *tm);
struct tm *gmtime(const time_t *time);

time_t time(time_t *time);
double difftime(time_t end, time_t start);

#endif
