#include <novino/spawn.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

int main(int argc, char *argv[])
{
    int status;
    pid_t pid;

    if(argc != 2)
    {
        printf("Usage: %s [pid]\n", argv[0]);
        return 0;
    }

    pid = atoi(argv[1]);
    status = signal(pid, 0);
    if(status < 0)
    {
        printf("%s: pid %d: %s\n", argv[0], pid, strerror(errno));
        return 1;
    }

    return 0;
}
