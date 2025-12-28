#include <novino/syscalls.h>
#include <string.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
    int status;

    if(argc != 2)
    {
        printf("Usage: %s [target]\n", argv[0]);
        return 0;
    }

    status = sys_umount(argv[1]);
    if(status < 0)
    {
        printf("Unmounting failed: %s\n", strerror(status));
        return 1;
    }

    printf("Successfully unmounted /%s\n", argv[1]);
    return 0;
}
