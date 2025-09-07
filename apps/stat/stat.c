#include <string.h>
#include <nonstd.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>

int main(int argc, char *argv[])
{
    int status;
    struct stat st;
    struct tm *tm;

    if(argc != 2)
    {
        printf("Usage: %s [file]\n", argv[0]);
        return 0;
    }

    status = stat(argv[1], &st);
    if(status < 0)
    {
        perror("Unable to stat file");
        return -1;
    }

    printf("File   : %s\n", argv[1]);
    printf("Size   : %d\n", st.st_size);
    printf("Mode   : %#o\n", st.st_mode);
    printf("Blocks : %d x %d\n", st.st_blocks, st.st_blksize);
    printf("Inode  : %d\n", st.st_ino);
    printf("UID    : %d\n", st.st_uid);
    printf("GID    : %d\n", st.st_gid);
    tm = gmtime(&st.st_atime);
    printf("Access : %s", asctime(tm));
    tm = gmtime(&st.st_mtime);
    printf("Modify : %s", asctime(tm));
    tm = gmtime(&st.st_ctime);
    printf("Change : %s", asctime(tm));

    return 0;
}
