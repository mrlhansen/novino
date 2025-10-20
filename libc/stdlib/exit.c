#include <novino/syscalls.h>
#include <_stdio.h>
#include <stdlib.h>

#define SLOTS_INCREMENT 20

static int slots = 0;
static int index = 0;

static struct {
    void (*func)();
} *list = 0;

int atexit(void (*func)())
{
    void *ptr;

    // allocate memory
    if(slots == index)
    {
        slots = slots + SLOTS_INCREMENT;
        ptr = realloc(list, sizeof(*list) * slots);
        if(ptr == NULL)
        {
            slots = slots - SLOTS_INCREMENT;
            return -1;
        }
        list = ptr;
    }

    // append to list
    list[index].func = func;
    index++;
    return 0;
}

void exit(int status)
{
    for(int i = index - 1; i >= 0; i--)
    {
        list[i].func();
    }

    __libc_fd_exit();

    sys_exit(status);
}
