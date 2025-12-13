#include <novino/syscalls.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

size_t getint(const char *data, const char *name)
{
    char buf[32];
    size_t len;

    len = sprintf(buf, "%s=", name);
    data = strstr(data, buf);
    if(!data)
    {
        return 0;
    }

    return atol(data + len);
}

int main(int argc, char *argv[])
{
    int bufsz = 4096;
    char *data;
    int len;

    data = malloc(bufsz);
    len = sys_sysinfo(1, 0, data, bufsz);
    if(len == bufsz)
    {
        printf("error: buffer too small");
        return 1;
    }

    size_t total = getint(data, "total");
    size_t free = getint(data, "free");
    size_t heap = getint(data, "heap");

    printf("Total : %lu MB\n", total/1000000);
    printf("Used  : %lu MB\n", (total - free)/1000000);
    printf("Free  : %lu MB\n", free/1000000);
    printf("Heap  : %lu MB\n", heap/1000000);

    return 0;
}
