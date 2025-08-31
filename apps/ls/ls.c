#include <unistd.h>
#include <nonstd.h>
#include <dirent.h>
#include <stdio.h>

static int oneflg = 0;
static int longflg = 0;
static int allflg = 0;

static void ls(const char *path, DIR *dp)
{
    struct dirent *dent;
    struct stat st;
    char buf[128];

    while(dent = readdir(dp), dent)
    {
        if(!allflg && dent->d_name[0] == '.')
        {
            continue;
        }
        if(oneflg)
        {
            printf("%s\n", dent->d_name);
        }
        else if(longflg)
        {
            sprintf(buf, "%s/%s", path, dent->d_name);
            stat(buf, &st);
            printf("%c%c%c%c%c%c%c%c%c%c % 4u % 4u %8lu %s\n",
                (dent->d_type == DT_DIR) ? 'd' : '-',
                (st.st_mode & 0400) ? 'r' : '-',
                (st.st_mode & 0200) ? 'w' : '-',
                (st.st_mode & 0100) ? 'x' : '-',
                (st.st_mode & 0040) ? 'r' : '-',
                (st.st_mode & 0020) ? 'w' : '-',
                (st.st_mode & 0010) ? 'x' : '-',
                (st.st_mode & 0004) ? 'r' : '-',
                (st.st_mode & 0002) ? 'w' : '-',
                (st.st_mode & 0001) ? 'x' : '-',
                st.st_uid,
                st.st_gid,
                st.st_size,
                dent->d_name
            );
        }
        else
        {
            printf("%s ", dent->d_name);
        }
    }

    if(!oneflg && !longflg)
    {
        printf("\n");
    }
}

int main(int argc, char *argv[])
{
    DIR *dp;
    int errflg = 0;
    int c;

    while(c = getopt(argc, argv, ":1la"), c != -1)
    {
        switch(c)
        {
            case '1':
                oneflg++;
                break;
            case 'l':
                longflg++;
                break;
            case 'a':
                allflg++;
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
        argv[argc++] = "."; // kind of hacky
    }

    for(int i = optind; i < argc; i++)
    {
        dp = opendir(argv[i]);
        if(dp == NULL)
        {
            printf("%s: %s: unable to open\n", argv[0], argv[i]); // perror();
            errflg = 1;
            continue;
        }
        ls(argv[i], dp);
        closedir(dp);
    }

    return errflg;
}
