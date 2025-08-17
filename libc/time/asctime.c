#include <time.h>
#include <stdio.h>

char *asctime(const struct tm *tm)
{
    static char result[26];

    static const char wday_name[7][4] = {
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
    };

    static const char mon_name[12][4] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };

    sprintf(result, "%.3s %.3s %2d %02d:%02d:%02d %d\n",
        (tm->tm_wday < 0 || tm->tm_wday > 6) ? "???" : wday_name[tm->tm_wday],
        (tm->tm_mon < 0 || tm->tm_mon > 11) ? "???" : mon_name[tm->tm_mon],
        tm->tm_mday, tm->tm_hour,
        tm->tm_min, tm->tm_sec,
        1900 + tm->tm_year);

    return result;
}
