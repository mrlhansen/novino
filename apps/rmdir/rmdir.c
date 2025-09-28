#include <string.h>
#include <unistd.h>
#include <stdio.h>

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
        perror("Unable to remove directory");
        return -1;
    }

    return 0;
}
