#include <kernel/time/time.h>
#include <kernel/debug.h>

static uint64_t frequency;

uint64_t rdtsc()
{
    uint32_t hi, lo;
    asm volatile("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
}

void tsc_calibrate()
{
    uint64_t first, second;

    timer_wait();
    first = rdtsc();
    timer_sleep(10);
    second = rdtsc();

    frequency = (second - first);
    frequency = (100UL * frequency);
}

uint64_t tsc_timestamp()
{
    uint64_t ts, rm;

    ts = rdtsc();
    rm = (ts % frequency);
    ts = TIME_NS * (ts / frequency);
    rm = (TIME_US * rm) / (frequency / TIME_MS);
    ts = ts + rm;

    return ts;
}

uint64_t tsc_frequency()
{
    return frequency;
}
