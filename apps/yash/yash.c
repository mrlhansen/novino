#include <novino/termio.h>
#include <novino/spawn.h>
#include <novino/stat.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>

extern char **environ;
static char *cmdv[32];
static char cmds[256];
static char cwd[256];

static int hist_pos;
static int hist_max;
static int hist_len;
static char **hist;

static int parse_cmdline(char *str)
{
    char *buf = cmds;
    char *start;

    int escape = 0;
    int quote = 0;
    int count = 0;

    while(*str)
    {
        while(isspace(*str))
        {
            str++;
        }

        if(*str == '\0')
        {
            break;
        }

        start = buf;

        while(*str)
        {
            if(isspace(*str) && !quote && !escape)
            {
                break;
            }

            if(*str == '\0')
            {
                break;
            }

            if(*str == '\\')
            {
                if(!escape)
                {
                    escape = 1;
                    str++;
                    continue;
                }
            }

            if((*str == '"' || *str == '\'') && !escape)
            {
                if(quote)
                {
                    if(quote == *str)
                    {
                        quote = 0;
                    }
                }
                else
                {
                    quote = *str;
                }

                str++;
                continue;
            }

            *buf++ = *str;
            escape = 0;
            str++;
        }

        if(buf > start)
        {
            *buf++ = '\0';
            cmdv[count++] = start;
        }
    }

    if(quote || escape)
    {
        return -1;
    }

    cmdv[count] = 0;
    return count;
}

static char *yash_gets(char *str)
{
    static int flags = 0;
    static char buf[64];

    int hpos = hist_len; // local history position
    int hoff = 0;        // offset in history ring
    int cont = 1;        // continue
    int pos = 0;         // position in current command string
    int len = 0;         // length of current command string
    int ch;

    // history offset
    if(hist_len == hist_max)
    {
        hoff = hist_pos;
    }

    // terminal flags
    if(!flags)
    {
        tiogetflags(&flags);
    }
    tiosetflags(TIONOBUF | TIOCURSOR | TIOWRAP);

    // null terminate
    *str = '\0';

    // read command
    while(cont)
    {
        char *seq = tiogets(buf, sizeof(buf));
        if(seq == 0)
        {
            return NULL;
        }

        while(*seq)
        {
            ch = *seq++;

            if(ch == '\e') // escape sequence
            {
                char *hstr;
                int a, b;

                seq -= 1;
                seq += tiogetescape(seq, &ch, &a, &b);

                if(ch == 'A') // up arrow
                {
                    if(hpos > 0)
                    {
                        hpos--;
                        hstr = hist[(hoff + hpos) % hist_max];
                        if(pos > 0)
                        {
                            printf("\e[%dD", pos);
                        }
                        printf("%-*s", len, hstr);
                        pos = strlen(hstr);
                        if(pos < len)
                        {
                            printf("\e[%dD", len - pos);
                        }
                        strcpy(str, hstr);
                        len = pos;
                    }
                }
                else if(ch == 'B') // down arrow
                {
                    if(hpos < hist_len)
                    {
                        hpos++;
                        hstr = hist[(hoff + hpos) % hist_max];
                        if(pos > 0)
                        {
                            printf("\e[%dD", pos);
                        }
                        if(hpos == hist_len)
                        {
                            if(len > 0)
                            {
                                printf("%*s\e[%dD", len, "\0", len);
                            }
                            *str = '\0';
                            len = 0;
                            pos = 0;
                        }
                        else
                        {
                            printf("%-*s", len, hstr);
                            pos = strlen(hstr);
                            if(pos < len)
                            {
                                printf("\e[%dD", len - pos);
                            }
                            strcpy(str, hstr);
                            len = pos;
                        }
                    }
                }
                else if(ch == 'C') // right arrow
                {
                    if(pos < len)
                    {
                        printf("\e[C");
                        pos++;
                    }
                }
                else if(ch == 'D') // left arrow
                {
                    if(pos > 0)
                    {
                        printf("\e[D");
                        pos--;
                    }
                }
                else if(ch == 'H') // beginning of line (home)
                {
                    if(pos > 0)
                    {
                        printf("\e[%dD", pos);
                        pos = 0;
                    }
                }
                else if(ch == 'F') // end of line (end)
                {
                    if(pos < len)
                    {
                        printf("\e[%dC", len - pos);
                        pos = len;
                    }
                }
                else if(ch == '~')
                {
                    if(a == 3) // delete
                    {
                        if(pos < len)
                        {
                            memmove(str + pos, str + pos + 1, len - pos);
                            len--;
                            str[len] = '\0';
                            printf("%s \e[%dD", str + pos, len - pos + 1);
                        }
                    }
                }
            }
            else if(ch == '\n') // new line
            {
                printf("\e[%dC", len - pos);
                putchar(ch);
                cont = 0;
                break;
            }
            else if(ch == '\t') // tab
            {

            }
            else if(ch == '\b') // backspace
            {
                if(pos < len)
                {
                    if(pos > 0)
                    {
                        memmove(str + pos - 1, str + pos, len - pos);
                        pos--;
                        len--;
                        str[len] = '\0';
                        printf("\b%s \e[%dD", str + pos, len - pos + 1);
                    }
                }
                else if(pos > 0)
                {
                    printf("\b \b");
                    pos--;
                    len--;
                    str[len] = '\0';
                }
            }
            else if(ch >= 32) // normal char
            {
                if(pos < len)
                {
                    memmove(str + pos + 1, str + pos, len - pos + 1);
                    str[pos] = ch;
                    printf("%s\e[%dD", str + pos, len - pos);
                    pos++;
                    len++;
                    str[len] = '\0';
                }
                else
                {
                    putchar(ch);
                    str[pos] = ch;
                    pos++;
                    len++;
                    str[len] = '\0';
                }
            }
        }

        fflush(stdout);
    }

    // restore flags
    tiosetflags(flags);

    // push command to history
    if(*str)
    {
        if(hist_len)
        {
            pos = (hist_pos + hist_max - 1) % hist_max;
            if(strcmp(str, hist[pos]) == 0)
            {
                return str; // do not store repeat of previous command
            }
        }

        if(hist[hist_pos])
        {
            free(hist[hist_pos]);
        }
        else
        {
            hist_len++;
        }

        hist[hist_pos] = calloc(1, strlen(str));
        strcpy(hist[hist_pos], str);
        hist_pos = (hist_pos + 1) % hist_max;
    }

    return str;
}

static void setpwd()
{
    getcwd(cwd, sizeof(cwd));
    setenv("PWD", cwd, 1);
}

static void yash_export(int argc, char *argv[])
{
    char *pos;
    int n = 0;

    if(argc == 1)
    {
        while(environ[n])
        {
            printf("%s\n", environ[n]);
            n++;
        }
    }
    else
    {
        for(int i = 1; i < argc; i++)
        {
            pos = strchr(argv[i], '=');
            if(pos == NULL)
            {
                printf("%s: %s: invalid environment variable\n", argv[0], argv[i]);
            }
            else
            {
                *pos = '\0';
                setenv(argv[i], pos+1, 1);
            }
        }
    }
}

static void yash_unset(int argc, char *argv[])
{
    for(int i = 1; i < argc; i++)
    {
        unsetenv(argv[i]);
    }
}

static void yash_cd(int argc, char *argv[])
{
    char *path;

    if(argc == 1)
    {
        path = "/";
    }
    else if(argc == 2)
    {
        path = argv[1];
    }
    else
    {
        printf("%s: too many arguments\n", argv[0]);
        return;
    }

    if(chdir(path) < 0)
    {
        printf("%s: %s: could not open\n", argv[0], path); // perror
    }
    else
    {
        setpwd();
    }
}

static void yash_echo(int argc, char *argv[])
{
    for(int i = 1; i < argc; i++)
    {
        printf("%s ", argv[i]);
    }
    printf("\n");
}

static void yash_history(int argc, char *argv[])
{
    int len = hist_len;
    int pos = 0;

    if(hist_len == hist_max)
    {
        pos = hist_pos;
    }

    for(int i = 0; i < len; i++)
    {
        printf("% 3d  %s\n", i, hist[pos]);
        pos = (pos + 1) % hist_max;
    }
}

static int run_builtin(int argc, char *argv[])
{
    if(strcmp(argv[0], "echo") == 0)
    {
        yash_echo(argc, argv);
        return 0;
    }
    else if(strcmp(argv[0], "export") == 0)
    {
        yash_export(argc, argv);
        return 0;
    }
    else if(strcmp(argv[0], "unset") == 0)
    {
        yash_unset(argc, argv);
        return 0;
    }
    else if(strcmp(argv[0], "clear") == 0)
    {
        printf("\e[2J");
        fflush(stdout);
        return 0;
    }
    else if(strcmp(argv[0], "cd") == 0)
    {
        yash_cd(argc, argv);
        return 0;
    }
    else if(strcmp(argv[0], "history") == 0)
    {
        yash_history(argc, argv);
        return 0;
    }
    else if(strcmp(argv[0], "exit") == 0)
    {
        exit(0);
    }
    return -1;
}

static int run_external(int argc, char *argv[])
{
    const char *path;
    char filename[128];
    int status;
    long pid;

    path = getenv("PATH");
    if(path == NULL)
    {
        return -1;
    }

    sprintf(filename, "%s/%s.elf", path, argv[0]);
    status = stat(filename, 0);
    if(status == 0) // should check exec bit
    {
        pid = spawnv(filename, argv);
        if(pid < 0)
        {
            printf("%s: cannot execute file\n", argv[0]);
            return 0;
        }
        wait(pid, &status);
        return 0;
    }

    return -1;
}

int main(int argc, char *argv[])
{
    char cmdline[128];
    int count;
    int status;

    setpwd();

    hist_max = 100;
    hist_pos = 0;
    hist_len = 0;
    hist = calloc(hist_max, sizeof(char*));

    while(1)
    {
        printf("\e[94myash\e[0m:\e[32m%s\e[0m$ ", cwd);
        fflush(stdout);

        if(yash_gets(cmdline) == NULL)
        {
            break;
        }

        count = parse_cmdline(cmdline);
        if(count <= 0)
        {
            if(count < 0)
            {
                printf("invalid command: %s\n", cmdline);
            }
            continue;
        }

        status = run_builtin(count, cmdv);
        if(status == 0)
        {
            continue;
        }

        status = run_external(count, cmdv);
        if(status == 0)
        {
            continue;
        }

        printf("%s: command not found\n", cmdv[0]);
    }

    return 0;
}
