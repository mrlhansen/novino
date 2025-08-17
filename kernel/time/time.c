#include <kernel/time/time.h>
#include <kernel/time/hpet.h>
#include <kernel/time/tsc.h>
#include <kernel/x86/cpuid.h>
#include <kernel/debug.h>
#include <time.h>

static volatile uint64_t ticks = 0;
static uint8_t tm_source; // Timer source
static uint8_t ts_source; // Timestamp source
static time_t btu; // Unix timestamp at boot
static time_t bts; // System timestamp at boot

void timer_handler(int gsi, void *data)
{
    ticks++;
}

uint64_t system_timestamp()
{
    if(ts_source == TSC)
    {
        return tsc_timestamp();
    }

    if(ts_source == HPET)
    {
        return hpet_timestamp();
    }

    return (1000000UL * ticks);
}

int gettime(timeval_t *tv)
{
    uint64_t cts;

    cts = system_timestamp();
    cts = (cts - bts);
    tv->tv_sec = btu + (cts / TIME_NS);
    tv->tv_nsec = (cts % TIME_NS);

    return 0;
}

void timer_wait()
{
    uint64_t current = ticks;
    while(current == ticks);
}

void timer_sleep(uint64_t ms)
{
    ms += ticks;
    while(ticks < ms);
}

void time_init()
{
    struct tm *time;
    int status;

    // Initialize timer
    status = hpet_init();
    if(status == 0)
    {
        tm_source = HPET;
        kp_info("time", "system clock uses HPET at %u Hz", TIMER_FREQUENCY);
    }
    else
    {
        pit_init();
        tm_source = PIT;
        kp_info("time", "system clock uses PIT at %u Hz", TIMER_FREQUENCY);
    }
    ts_source = tm_source;

    // Initialize TSC
    status = cpuid_tsc_invariant();
    if(status)
    {
        tsc_calibrate();
        ts_source = TSC;
        kp_info("time", "found invariant TSC at %lu kHz", tsc_frequency()/1000);
    }

    // Current time
    btu = rtc_get_timestamp();
    bts = system_timestamp();
    time = gmtime(&btu);
    kp_info("time", "current time: %.24s", asctime(time));
}
