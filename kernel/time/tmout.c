#include <kernel/time/tmout.h>
#include <kernel/time/time.h>

void tmout_init(tmout_t *tmo, int ms)
{
    tmo->period = NANOSECONDS(ms, TIME_MS);
    tmo->end = system_timestamp() + tmo->period;
    tmo->expired = false;
}

void tmout_update(tmout_t *tmo)
{
    tmo->expired = (system_timestamp() >= tmo->end);
}
