#include <time.h>
#include <stdio.h>

static const int days_per_month[2][12] = {
    {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
    {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
};

#define LEAP_YEAR(y)        ((((y) % 4) == 0 && ((y) % 100) != 0) || ((y) % 400) == 0)
#define DAYS_IN_YEAR(y)     (LEAP_YEAR(y+1900) ? 366 : 365)
#define DAYS_PER_MONTH(y,m) (LEAP_YEAR(y+1900) ? days_per_month[1][m] : days_per_month[0][m])

#define SECONDS_PER_DAY    86400L
#define SECONDS_PER_HOUR   3600L
#define SECONDS_PER_MINUTE 60L

struct tm *gmtime(const time_t *time)
{
    long days, tmp, year, month;
    static struct tm tm;
    time_t ts;

    month = 0;
    year = 70;
    ts = *time;

    days = (ts / SECONDS_PER_DAY);
    if(ts < 0)
    {
        days--;
    }
    ts -= (days * SECONDS_PER_DAY);

    tm.tm_wday = (days + 4) % 7;
    if(tm.tm_wday < 0)
    {
        tm.tm_wday += 7;
    }

    while(days > 0)
    {
        tmp = DAYS_IN_YEAR(year);
        if(days < tmp)
        {
            break;
        }
        days -= tmp;
        year++;
    }

    while(days < 0)
    {
        days += DAYS_IN_YEAR(year);
        year--;
    }

    tm.tm_yday = days;
    tm.tm_isdst = 0;

    while(1)
    {
        tmp = DAYS_PER_MONTH(year, month);
        if(days < tmp)
        {
            break;
        }
        days -= tmp;
        month++;
    }

    tm.tm_year = year;
    tm.tm_mon = month;
    tm.tm_mday = days+1;
    ts = (ts % SECONDS_PER_DAY);
    tm.tm_hour = (ts / SECONDS_PER_HOUR);
    ts = (ts % SECONDS_PER_HOUR);
    tm.tm_min = (ts / SECONDS_PER_MINUTE);
    ts = (ts % SECONDS_PER_MINUTE);
    tm.tm_sec = ts;

    return &tm;
}
