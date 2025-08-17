#include <unistd.h>
#include <string.h>
#include <stdio.h>

char *optarg;
int opterr = 1;
int optind = 0;
int optopt;

int getopt(int argc, char *const argv[], const char *optstring)
{
    static int optpos = 1;
    char *arg;
    char *opt;
    int ch;

    // reset
    if(optind == 0)
    {
        optind = 1;
        optpos = 1;
    }

    // validate state
    if(optind >= argc || argv[optind] == 0)
    {
        return -1;
    }

    arg = argv[optind];

    if(arg[0] != '-')
    {
        return -1;
    }

    if(arg[1] == '\0')
    {
        return -1;
    }

    if(strcmp(arg, "--") == 0)
    {
        optind++;
        return -1;
    }

    // parse arguments
    ch = arg[optpos];
    opt = strchr(optstring, ch);

    if(!opt)
    {
        optopt = ch;
        if(opterr && *optstring != ':')
        {
            printf("%s: illegal option: %s\n", argv[0], optopt);
        }

        if(arg[++optpos] == '\0')
        {
            optind++;
            optpos = 1;
        }

        return '?';
    }

    if(opt[1] == ':')
    {
        if(arg[optpos+1])
        {
            optarg = arg + optpos + 1;
            optind++;
            optpos = 1;
            return ch;
        }
        else if(argv[optind+1])
        {
            optarg = argv[optind+1];
            optind += 2;
            optpos = 1;
            return ch;
        }
        else
        {
            if(opterr && *optstring != ':')
            {
                printf("%s: option requires an argument: %s\n", argv[0], optopt);
            }
            if(arg[++optpos] == '\0')
            {
                optind++;
                optpos = 1;
            }
            return (*optstring == ':') ? ':' : '?';
        }
    }
    else
    {
        if(arg[++optpos] == '\0')
        {
            optind++;
            optpos = 1;
        }
        return ch;
    }
}
