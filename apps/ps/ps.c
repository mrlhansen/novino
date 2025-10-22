#include <novino/syscalls.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

static int tflg = 0;
static int aflg = 0;

typedef struct {
    char name[32];
    size_t tid;
    size_t prio;
    size_t state;
    size_t cpu;
} thread_t;

typedef struct {
    char name[32];
    size_t pid;
    size_t ppid;
    size_t state;
    size_t uid;
    size_t gid;
    size_t mem;
    size_t cpu;
    size_t nth;
    thread_t threads[];
} process_t;

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

char *getstr(const char *data, const char *name, char *str)
{
    const char *d;
    char *s;
    char buf[32];
    size_t len;

    len = sprintf(buf, "%s=", name);
    data = strstr(data, buf);
    if(!data)
    {
        return 0;
    }

    d = data + len;
    s = str;

    while(*d && *d != ';')
    {
        *s++ = *d++;
    }

    *s = '\0';
    return str;
}

int thread_state(thread_t *t)
{
    int st;
    switch(t->state)
    {
        case 0:
            st = 'Q';
            break;
        case 1:
            st = 'R';
            break;
        case 2:
            st = 'S';
            break;
        case 3:
            st = 'B';
            break;
        case 4:
            st = 'W';
            break;
        case 5:
            st = 'T';
            break;
        default:
            st = '?';
            break;
    }
    return st;
}

int process_state(process_t *p)
{
    int Q = 0;
    int R = 0;
    int S = 0;
    int B = 0;
    int W = 0;
    int T = 0;
    int c;

    for(int i = 0; i < p->nth; i++)
    {
        c = thread_state(p->threads + i);
        switch(c)
        {
            case 'Q':
                Q++;
                break;
            case 'R':
                R++;
                break;
            case 'S':
                S++;
                break;
            case 'B':
                B++;
                break;
            case 'W':
                W++;
                break;
            case 'T':
                T++;
                break;
            default:
                break;
        }
    }

    if(R)
    {
        return 'R';
    }

    if(Q)
    {
        return 'Q';
    }

    if(S == p->nth)
    {
        return 'S';
    }

    if(B == p->nth)
    {
        return 'B';
    }

    if(W == p->nth)
    {
        return 'W';
    }

    if(T == p->nth)
    {
        return 'T';
    }

    return 'M';
}

void print_pid(int pid)
{
    int bufsz = 4096;
    char *data;

    process_t *p;
    thread_t *t;
    char *th;
    int len, nth;

    data = malloc(bufsz);
    len = sys_sysinfo(3, pid, data, bufsz);
    if(len == bufsz)
    {
        printf("error: pid buffer too small");
        exit(1);
    }

    nth = getint(data, "threads");
    p = malloc(sizeof(process_t) + nth * sizeof(thread_t));

    getstr(data, "name", p->name);
    p->pid = pid;
    p->ppid = getint(data, "ppid");
    p->state = getint(data, "state");
    p->uid = getint(data, "uid");
    p->gid = getint(data, "gid");
    p->mem = getint(data, "memsz");
    p->cpu = 0;
    p->nth = nth;

    th = strstr(data, "tid");
    nth = 0;

    while(th)
    {
        t = p->threads + nth++;

        getstr(th, "name", t->name);
        t->tid = getint(th, "tid");
        t->prio = getint(th, "priority");
        t->state = getint(th, "state");
        t->cpu = getint(th, "cputime");
        p->cpu += t->cpu;

        th = strstr(th + 3, "tid");
    }

    free(data);

    if(tflg)
    {
        printf("%-6d %-6d %-6s %-16s %-8lu %-8lu %-6c\n",
            p->pid,
            p->ppid,
            "",
            p->name,
            p->mem,
            p->cpu / 1000000,
            process_state(p)
        );
    }
    else
    {
        printf("%-6d %-6d %-16s %-8lu %-8lu %-6c\n",
            p->pid,
            p->ppid,
            p->name,
            p->mem,
            p->cpu / 1000000,
            process_state(p)
        );
    }

    if(!tflg)
    {
        free(p);
        return;
    }

    for(int i = 0; i < p->nth; i++)
    {
        t = p->threads + i;
        printf("%-6d %-6d %-6d %-16s %-8s %-8lu %-6c\n",
            p->pid,
            p->ppid,
            t->tid,
            t->name,
            "",
            t->cpu / 1000000,
            thread_state(t)
        );
    }

    free(p);
}

int main(int argc, char *argv[])
{
    int bufsz = 4096;
    char *data;

    char *tok;
    int errflg = 0;
    int c, pid;
    int len;

    while(c = getopt(argc, argv, ":ta"), c != -1)
    {
        switch(c)
        {
            case 't':
                tflg++;
                break;
            case 'a':
                aflg++;
                break;
            default:
                printf("unrecognized option: '-%c'\n", optopt);
                errflg++;
                break;
        }
    }

    if(errflg)
    {
        return -1;
    }

    data = malloc(bufsz);
    len = sys_sysinfo(2, 0, data, bufsz);
    if(len == bufsz)
    {
        printf("error: list buffer too small");
        return 1;
    }

    if(tflg)
    {
        printf("%-6s %-6s %-6s %-16s %-8s %-8s %-6s\n",
            "PID",
            "PPID",
            "TID",
            "NAME",
            "MEM",
            "CPU",
            "STATE"
        );
    }
    else
    {
        printf("%-6s %-6s %-16s %-8s %-8s %-6s\n",
            "PID",
            "PPID",
            "NAME",
            "MEM",
            "CPU",
            "STATE"
        );
    }

    tok = strtok(data, ";");
    while(tok)
    {
        pid = atoi(tok);
        if(pid || aflg)
        {
            print_pid(pid);
        }
        tok = strtok(NULL, ";");
    }
}
