#pragma once

#include <kernel/lists.h>

typedef struct timer {
    size_t deadline;
    size_t period;
    void (*callback)(struct timer*);
    void *data;
    link_t link;
} timer_t;

void timer_tick();
int timer_start(timer_t *tm);
void timer_cancel(timer_t *tm);
