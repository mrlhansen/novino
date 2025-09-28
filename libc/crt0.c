#ifdef __KERNEL__

int errno;

#else

#include <_stdio.h>
#include <_stdlib.h>

static int exitcode;
extern int main(int argc, char *argv[]);

char **environ;
FILE *stdin;
FILE *stdout;
int errno;

void __start(char **argv, char **envp)
{
    int argc = 0;
    while(argv[argc])
    {
        argc++;
    }

    environ = envp;
    __libc_heap_init();

    stdin = __libc_fd_alloc(0);
    stdin->flags = (F_READ | F_TEXT);
    stdout = __libc_fd_alloc(1);
    stdout->flags = (F_WRITE | F_TEXT);

    exitcode = main(argc, argv);
    __libc_fd_exit();
    exit(exitcode);
}

#endif
