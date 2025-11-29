#include <novino/stat.h>
#include <string.h>
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
        printf("%s: %s: %s\n", argv[0], argv[1], strerror(errno));
        return 1;
    }

    printf("File   : %s\n", argv[1]);
    printf("Size   : %lu\n", st.st_size);
    printf("Mode   : %04o\n", st.st_mode & 0xFFF);
    printf("Blocks : %lu x %u\n", st.st_blocks, st.st_blksize);
    printf("Inode  : %u\n", st.st_ino);
    printf("UID    : %u\n", st.st_uid);
    printf("GID    : %u\n", st.st_gid);
    tm = gmtime(&st.st_atime);
    printf("Access : %s", asctime(tm));
    tm = gmtime(&st.st_mtime);
    printf("Modify : %s", asctime(tm));
    tm = gmtime(&st.st_ctime);
    printf("Change : %s", asctime(tm));

    // Add links and file type

    return 0;
}
