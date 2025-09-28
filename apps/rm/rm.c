#include <_syscall.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

int main(int argc, char *argv[])
{
    int status;
    int errflag = 0;

    if(argc == 1)
    {
        printf("Usage: %s file ...\n", argv[0]);
        return 0;
    }

    for(int i = 1; i < argc; i++)
    {
        status = sys_remove(argv[i]);
        if(status < 0)
        {
            errno = -status;
            printf("%s: %s: %s\n", argv[0], argv[i], strerror(errno));
            errflag = 1;
        }
    }

    return errflag;
}
