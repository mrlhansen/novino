#pragma once

#include <kernel/types.h>

uint64_t rdtsc();
void tsc_calibrate();
uint64_t tsc_timestamp();
uint64_t tsc_frequency();
