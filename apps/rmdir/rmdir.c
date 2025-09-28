#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

int main(int argc, char *argv[])
{
    int status;

    if(argc != 2)
    {
        printf("Usage: %s [directory]\n", argv[0]);
        return 0;
    }

    status = rmdir(argv[1]);
    if(status < 0)
    {
        printf("%s: %s: %s\n", argv[0], argv[1], strerror(errno));
        return 1;
    }

    return 0;
}
