#include <unistd.h>
#include <string.h>
#include <stdio.h>

static int nflg = 0;
static char buf[512];

static void cat(FILE *fp)
{
    int line = 1;
    int size;

    while(size = fread(buf, 1, 512, fp), size > 0)
    {
        buf[size] = '\0';
        if(nflg)
        {
            printf("% 4d %s", line, buf);
        }
        else
        {
            printf("%s", buf);
        }
        line++;
    }
}

int main(int argc, char *argv[])
{
    FILE *fp;
    int errflg = 0;
    int c;

    // tmp
    nflg = 0;

    while(c = getopt(argc, argv, ":n"), c != -1)
    {
        switch(c)
        {
            case 'n':
                nflg++;
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

    if(optind == argc)
    {
        cat(stdin);
        return 0;
    }

    for(int i = optind; i < argc; i++)
    {
        if(strcmp(argv[i], "-") == 0)
        {
            cat(stdin);
        }
        else
        {
            fp = fopen(argv[i], "r");
            if(fp == NULL)
            {
                printf("%s: %s: unable to open\n", argv[0], argv[i]); // perror();
                errflg = 1;
                continue;
            }

            setvbuf(fp, 0, _IOLBF, 0);
            cat(fp);
            fclose(fp);
        }
    }

    return errflg;
}
