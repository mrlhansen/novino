#pragma once

#include <kernel/types.h>

#define TIMER_FREQUENCY 1000

#define TIME_FS 1000000000000000UL // Femtoseconds
#define TIME_PS 1000000000000UL    // Picoseconds
#define TIME_NS 1000000000UL       // Nanoseconds
#define TIME_US 1000000UL          // Microseconds
#define TIME_MS 1000UL             // Miliseconds

enum {
    PIT,
    HPET,
    TSC
};

typedef struct {
    uint64_t tv_sec;
    uint64_t tv_nsec;
} timeval_t;

// rtc.c
uint64_t rtc_get_timestamp();

// pit.c
void pit_init();

// time.c
void timer_wait();
void timer_sleep(uint64_t ms);
void timer_handler(int, void*);
uint64_t system_timestamp();
int gettime(timeval_t *tv);
void time_init();
