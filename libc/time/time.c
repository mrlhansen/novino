#include <kernel/time/time.h>
#include <novino/syscalls.h>
#include <time.h>

time_t time(time_t *time)
{
    timeval_t tv;
    sys_gettime(&tv);
    if(time)
    {
        *time = tv.tv_sec;
    }
    return tv.tv_sec;
}
