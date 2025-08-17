#include <nonstd.h>
#include <unistd.h>
#include <stdio.h>

// The init process cannot read from stdin

#define SH_MAX 2

typedef struct {
    FILE *vts;
    long pid;
    int id;
} sh_t;

void spawn_shell(sh_t *sh)
{
    char path[16];
    char env_vts[16];

    // open virtual terminal
    if(sh->vts == NULL)
    {
        sprintf(path, "/devices/vts%d", sh->id);
        sh->vts = fopen(path, "r+");
        if(sh->vts == NULL)
        {
            printf("init: failed to open %s\n", path);
            return;
        }
        setvbuf(sh->vts, 0, _IOLBF, 0);
    }

    // arguments
    sprintf(env_vts, "VTS=%d", sh->id);

    char *argv[] = {
        "/initrd/apps/yash.elf",
        0
    };

    char *envp[] = {
        "PATH=/initrd/apps",
        env_vts,
        0
    };

    // execute shell
    fprintf(sh->vts, "init: launching %s on /devices/vts%d\n", argv[0], sh->id);
    sh->pid = spawnvef(argv[0], argv, envp, sh->vts, sh->vts);
    if(sh->pid < 0)
    {
        fprintf(sh->vts, "init: failed to spawn shell\n");
    }
}

int main(int argc, char *argv[])
{
    sh_t sh[SH_MAX];
    int status;
    long pid;

    pid = getpid();
    if(pid != 1)
    {
        printf("%s: not running as pid 1, exiting\n", argv[0]);
        return -1;
    }

    for(int i = 0; i < SH_MAX; i++)
    {
        sh[i].vts = 0;
        sh[i].id = i + 1;
        spawn_shell(sh+i);
    }

    while(1)
    {
        pid = wait(0, &status);
        for(int i = 0; i < SH_MAX; i++)
        {
            if(pid == sh[i].pid)
            {
                spawn_shell(sh+i);
                break;
            }
        }
    }
}
