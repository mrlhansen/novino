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

    status = mkdir(argv[1], 0777);
    if(status < 0)
    {
        perror("Unable to create directory");
        return -1;
    }

    return 0;
}
