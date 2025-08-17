#include <time.h>

static const int days_per_month[2][12] = {
    {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
    {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};

static const int days_by_month[2][12] = {
    {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334},
    {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335}
};

#define LEAP_YEAR(y)        ((((y) % 4) == 0 && ((y) % 100) != 0) || ((y) % 400) == 0)
#define DAYS_IN_YEAR(y)     (LEAP_YEAR(y+1900) ? 366 : 365)
#define DAYS_PER_MONTH(y,m) (LEAP_YEAR(y+1900) ? days_per_month[1][m] : days_per_month[0][m])
#define DAYS_BY_MONTH(y,m)  (LEAP_YEAR(y+1900) ? days_by_month[1][m] : days_by_month[0][m])

#define SECONDS_PER_DAY    86400L
#define SECONDS_PER_HOUR   3600L
#define SECONDS_PER_MINUTE 60L

static void normalize(int *a, int *b, int base)
{
    int value, lo, hi;

    lo = *a;
    hi = *b;

    if(lo < 0)
    {
        value = -lo;
        if(value % base)
        {
            lo = base - (value % base);
            hi = hi - 1 - (value / base);
        }
        else
        {
            lo = 0;
            hi = hi - (value / base);
        }
    }
    else if(lo >= base)
    {
        value = lo;
        lo = (value % base);
        hi = hi + (value / base);
    }

    *a = lo;
    *b = hi;
}

time_t mktime(struct tm *tm)
{
    time_t timestamp;
    long days, tmp;

    normalize(&tm->tm_sec, &tm->tm_min, 60);
    normalize(&tm->tm_min, &tm->tm_hour, 60);
    normalize(&tm->tm_hour, &tm->tm_mday, 24);
    normalize(&tm->tm_mon, &tm->tm_year, 12);

    while(tm->tm_mday <= 0)
    {
        tm->tm_mon--;

        if(tm->tm_mon == -1)
        {
            tm->tm_mon = 11;
            tm->tm_year--;
        }

        tm->tm_mday += DAYS_PER_MONTH(tm->tm_year, tm->tm_mon);
    }

    while(1)
    {
        tmp = DAYS_PER_MONTH(tm->tm_year, tm->tm_mon);

        if(tm->tm_mday <= tmp)
        {
            break;
        }

        tm->tm_mday -= tmp;
        tm->tm_mon++;

        if(tm->tm_mon >= 12)
        {
            tm->tm_mon = 0;
            tm->tm_year++;
        }
    }

    days = tm->tm_mday - 1;
    days += DAYS_BY_MONTH(tm->tm_year, tm->tm_mon);
    tm->tm_yday = days;

    if(tm->tm_year > 70)
    {
        for(int i = 70; i < tm->tm_year; i++)
        {
            days += DAYS_IN_YEAR(i);
        }
    }
    else if(tm->tm_year < 70)
    {
        for(int i = tm->tm_year; i < 70; i++)
        {
            days -= DAYS_IN_YEAR(i);
        }
    }

    tm->tm_wday = (days + 4) % 7;
    if(tm->tm_wday < 0)
    {
        tm->tm_wday += 7;
    }

    timestamp = days * SECONDS_PER_DAY;
    timestamp += tm->tm_hour * SECONDS_PER_HOUR;
    timestamp += tm->tm_min * SECONDS_PER_MINUTE;
    timestamp += tm->tm_sec;

    return timestamp;
}
