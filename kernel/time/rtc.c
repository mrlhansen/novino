#include <kernel/time/time.h>
#include <kernel/x86/ioports.h>
#include <time.h>

static uint8_t bcd2dec(uint8_t bcd)
{
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

static uint8_t rtc_read(uint8_t reg)
{
    outportb(0x70, reg);
    return inportb(0x71);
}

static void rtc_write(uint8_t reg, uint8_t value)
{
    outportb(0x70, reg);
    outportb(0x71, value);
}

static void rtc_wait()
{
    uint8_t status;
    do
    {
        status = rtc_read(0x0A);
    }
    while(status & 0x80);
}

uint64_t rtc_get_timestamp()
{
    int secs, mins, hours;
    int day, month, year;
    uint8_t v[6], w[6];
    uint8_t flags, pm;
    struct tm tm;
    time_t ts;

    // Read until data is consistent
    while(1)
    {
        flags = 0;
        rtc_wait();

        v[0] = rtc_read(0x00);
        v[1] = rtc_read(0x02);
        v[2] = rtc_read(0x04);
        v[3] = rtc_read(0x07);
        v[4] = rtc_read(0x08);
        v[5] = rtc_read(0x09);

        for(int i = 0; i < 6; i++)
        {
            if(v[i] != w[i])
            {
                flags = 1;
                w[i] = v[i];
            }
        }

        if(flags == 0)
        {
            break;
        }
    }

    // Copy data
    secs  = v[0];
    mins  = v[1];
    hours = v[2];
    day   = v[3];
    month = v[4];
    year  = v[5];

    // Hour format
    pm = (hours & 0x80);
    hours = (hours & 0x7F);

    // Register B
    flags = rtc_read(0x0B);

    // Convert from BCD
    if((flags & 0x04) == 0)
    {
        secs  = bcd2dec(secs);
        mins  = bcd2dec(mins);
        hours = bcd2dec(hours);
        day   = bcd2dec(day);
        month = bcd2dec(month);
        year  = bcd2dec(year);
    }

    // Hour format
    if((flags & 0x02) == 0)
    {
        if(pm)
        {
            hours = (hours + 12) % 24;
        }
    }

    // Adjust year
    year = 2000 + year;

    // Convert to timestamp
    tm.tm_hour = hours;
    tm.tm_min = mins;
    tm.tm_sec = secs;
    tm.tm_mday = day;
    tm.tm_mon = (month - 1);
    tm.tm_year = (year - 1900);

    ts = mktime(&tm);
    return ts;
}
