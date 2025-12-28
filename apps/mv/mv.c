#include <novino/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

static int vflg = 0;
char path[256];

char *basename(const char *path)
{
    char *s;

    s = strrchr(path, '/');
    if(s)
    {
        s++;
    }
    else
    {
        s = (char*)path;
    }

    return s;
}

int main(int argc, char *argv[])
{
    struct stat st;
    char *dest;
    int status;
    int errflg = 0;
    int nargs = 0;
    int c;

    while(c = getopt(argc, argv, ":v"), c != -1)
    {
        switch(c)
        {
            case 'v':
                vflg++;
                break;
            default:
                printf("unrecognized option: '-%c'\n", optopt);
                errflg++;
                break;
        }
    }

    if(errflg)
    {
        return 1;
    }

    nargs = argc - optind;
    if(nargs < 2)
    {
        printf("Usage: %s [options] source dest\n", argv[0]);
        return 0;
    }

    memset(&st, 0, sizeof(st));
    dest = argv[argc-1];
    status = stat(dest, &st);

    if(status < 0)
    {
        if(nargs > 2)
        {
            printf("%s: %s: is not a directory\n", argv[0], dest);
            return 1;
        }
    }
    else
    {
        if((st.st_mode & S_IFDIR) == 0)
        {
            if(nargs > 2)
            {
                printf("%s: %s: is not a directory\n", argv[0], dest);
                return 1;
            }
        }
    }

    if((st.st_mode & S_IFDIR) == 0)
    {
        if(vflg)
        {
            printf("%s -> %s\n", argv[optind], path);
        }

        status = rename(argv[optind], dest);
        if(status)
        {
            printf("%s: %s: %s\n", argv[0], dest, strerror(errno));
            errflg = 1;
        }

        return errflg;
    }

    for(int i = optind; i < argc - 1; i++)
    {
        sprintf(path, "%s/%s", dest, basename(argv[i]));
        if(vflg)
        {
            printf("%s -> %s\n", argv[i], path);
        }

        status = rename(argv[i], path);
        if(status)
        {
            printf("%s: %s: %s\n", argv[0], path, strerror(errno));
            errflg = 1;
        }
    }

    return errflg;
}
