#pragma once

#include <kernel/types.h>

typedef struct {
    uint64_t end;
    uint64_t period;
    bool expired;
} tmout_t;

void tmout_init(tmout_t *tmo, int ms);
void tmout_update(tmout_t *tmo);
