#include <novino/syscalls.h>
#include <string.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
    int status;

    if(argc != 4)
    {
        printf("Usage: %s [source] [fstype] [target]\n", argv[0]);
        return 0;
    }

    status = sys_mount(argv[1], argv[2], argv[3]);
    if(status < 0)
    {
        printf("Mounting failed: %s\n", strerror(status));
        return 1;
    }

    printf("Successfully mounted %s on /%s\n", argv[1], argv[3]);
    return 0;
}
