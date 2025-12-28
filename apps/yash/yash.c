#include <novino/termio.h>
#include <novino/spawn.h>
#include <novino/stat.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>

char *autocomplete(char *str, int suggest);

typedef struct {
    // Command
    int sep;         // Separator
    int argc;        // Argument count
    char *argv[32];  // Argument list
    char *ofs;       // Output file string
    char *ifs;       // Input file string
    // Child process
    pid_t pid;       // Process ID
    int wait;        // Wait in foreground
    int exitcode;    // Exit code
    int ofd;         // Output file descriptor
    int ifd;         // Input file descriptor
} args_t;

extern char **environ;
static char cmds[256];
static args_t args[32];
static char cwd[256];

static int hist_pos;
static int hist_max;
static int hist_len;
static char **hist;

static inline void print_shell()
{
    printf("\e[94myash\e[0m:\e[32m%s\e[0m$ ", cwd);
}

static int validate_special(const char *str)
{
    if(strcmp(str, "&") == 0)
    {
        return 1;
    }
    if(strcmp(str, "&&") == 0)
    {
        return 2;
    }
    if(strcmp(str, "|") == 0)
    {
        return 3;
    }
    if(strcmp(str, "||") == 0)
    {
        return 4;
    }
    if(strcmp(str, ";") == 0)
    {
        return 5;
    }
    if(strcmp(str, ">") == 0)
    {
        return 6;
    }
    if(strcmp(str, ">>") == 0)
    {
        return 7;
    }
    if(strcmp(str, "<") == 0)
    {
        return 8;
    }
    return -1;
}

static inline void reset_args(args_t *args)
{
    memset(args, 0, sizeof(*args));
    args->sep = 5;
    args->ofd = fileno(stdout);
    args->ifd = fileno(stdin);
    args->wait = 1;
}

static int parse_cmdline(char *str, args_t *args)
{
    args_t *cmd;
    char *start;
    char *buf;

    int special = 0;
    int escape = 0;
    int quote = 0;
    int count = 0;
    int expect = 0;

    buf = cmds;
    cmd = args;
    reset_args(cmd);

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

            if(strchr("&|;><", *str) && !escape && !quote)
            {
                if(!special)
                {
                    special = 1;
                    break;
                }
                else
                {
                    special = 2;
                }
            }
            else if(special)
            {
                break;
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
            *buf = '\0';

            if(special == 2)
            {
                special = validate_special(start);
                if(special < 0)
                {
                    return -1;
                }

                if(special >= 6 && special <= 8)
                {
                    expect = special;
                    special = 0;
                    continue;
                }

                if(!cmd->argc)
                {
                    return -1;
                }

                cmd->sep = special;
                special = 0;

                count++;
                buf = start;
                cmd = args + count;
                reset_args(cmd);
            }
            else
            {
                buf++;

                if(expect == 6)
                {
                    if(cmd->ofs)
                    {
                        return -1;
                    }
                    cmd->ofs = start;
                    expect = 0;
                    continue;
                }

                if(expect == 8)
                {
                    if(cmd->ifs)
                    {
                        return -1;
                    }
                    cmd->ifs = start;
                    expect = 0;
                    continue;
                }

                cmd->argv[cmd->argc] = start;
                cmd->argc++;
            }
        }
    }

    if(quote || escape || expect)
    {
        return -1;
    }

    if(cmd->argc)
    {
        count++;
    }

    cmd = args;

    for(int i = 0; i < count; i++)
    {
        int first = !i;
        int last  = !(count - i - 1);
        int sep   = cmd[i].sep;

        if(sep == 3)
        {
            if(cmd[i].ofs)
            {
                return -1;
            }

            if(!first)
            {
                if(cmd[i-1].ofs)
                {
                    return -1;
                }
            }

            if(!last)
            {
                if(cmd[i+1].ifs)
                {
                    return -1;
                }
            }
        }

        if(last)
        {
            if(!(sep == 1 || sep == 5))
            {
                return -1;
            }
        }

    }

    return count;
}

static char *yash_gets(char *str)
{
    static int flags = 0;
    static char buf[64];
    static char abuf[64];

    int hpos = hist_len; // local history position
    int hoff = 0;        // offset in history ring
    int cont = 1;        // continue
    int pos = 0;         // position in current command string
    int len = 0;         // length of current command string
    int tabz = 0;        // next tab press should show suggetions
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
                char *tmp;
                int start = 0;

                for(int i = pos - 1; i >= 0; i--)
                {
                    if(str[i] == ' ')
                    {
                        start = i + 1;
                        break;
                    }
                }

                if(!start)
                {
                    // no autocompletion for commands
                    continue;
                }

                memset(abuf, 0, sizeof(abuf));
                strncpy(abuf, str + start, pos - start);

                if(tabz)
                {
                    tmp = autocomplete(abuf, 1);
                    if(tmp)
                    {
                        print_shell();
                        printf("%s \e[%dD", str, len - pos + 1);
                    }
                    continue;
                }

                tmp = autocomplete(abuf, 0);
                if(tmp)
                {
                    seq = tmp;
                    continue;
                }

                tabz = 1;
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

            if(ch != '\t')
            {
                tabz = 0;
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

static int yash_export(int argc, char *argv[])
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
            if(!pos)
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

    return 0;
}

static int yash_unset(int argc, char *argv[])
{
    for(int i = 1; i < argc; i++)
    {
        unsetenv(argv[i]);
    }
    return 0;
}

static int yash_cd(int argc, char *argv[])
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
        return 1;
    }

    if(chdir(path) < 0)
    {
        printf("%s: %s: %s\n", argv[0], path, strerror(errno));
        return 1;
    }

    setpwd();
    return 0;
}

static int yash_echo(int argc, char *argv[])
{
    for(int i = 1; i < argc; i++)
    {
        printf("%s ", argv[i]);
    }
    printf("\n");
    return 0;
}

static int yash_history(int argc, char *argv[])
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

    return 0;
}

static int yash_clear(int argc, char *argv[])
{
    printf("\e[2J");
    fflush(stdout);
    return 0;
}

static int run_builtin(args_t *p)
{
    char **argv;
    char *cmd;
    int argc;

    cmd  = p->argv[0];
    argv = p->argv;
    argc = p->argc;

    if(strcmp(cmd, "echo") == 0)
    {
        p->exitcode = yash_echo(argc, argv);
        return 0;
    }
    else if(strcmp(cmd, "export") == 0)
    {
        p->exitcode = yash_export(argc, argv);
        return 0;
    }
    else if(strcmp(cmd, "unset") == 0)
    {
        p->exitcode = yash_unset(argc, argv);
        return 0;
    }
    else if(strcmp(cmd, "clear") == 0)
    {
        p->exitcode = yash_clear(argc, argv);
        return 0;
    }
    else if(strcmp(cmd, "cd") == 0)
    {
        p->exitcode = yash_cd(argc, argv);
        return 0;
    }
    else if(strcmp(cmd, "history") == 0)
    {
        p->exitcode = yash_history(argc, argv);
        return 0;
    }
    else if(strcmp(cmd, "true") == 0)
    {
        p->exitcode = 0;
        return 0;
    }
    else if(strcmp(cmd, "false") == 0)
    {
        p->exitcode = 1;
        return 0;
    }
    else if(strcmp(cmd, "exit") == 0)
    {
        exit(0);
    }

    return -1;
}

static int run_external(args_t *p)
{
    const char *path;
    char filename[128];
    int status;
    long pid;

    path = getenv("PATH");
    if(!path)
    {
        return -1;
    }

    sprintf(filename, "%s/%s.elf", path, p->argv[0]);
    status = stat(filename, 0);
    if(status < 0)
    {
        return -1;
    }

    pid = spawnvef(filename, p->argv, environ, p->ifd, p->ofd);
    if(pid < 0)
    {
        printf("%s: cannot execute file\n", p->argv[0]); // strerror
        return -1;
    }

    p->pid = pid;
    if(p->wait)
    {
        wait(pid, &status);
        p->exitcode = status;
    }

    return 0;
}

static int run_command(args_t *p)
{
    int status;

    status = run_builtin(p);
    if(!status)
    {
       return p->exitcode;
    }

    status = run_external(p);
    if(!status)
    {
       return p->exitcode;
    }

    printf("%s: command not found\n", p->argv[0]);
    return 127;
}

static int execute(int count, args_t *args)
{
    args_t *cmd;
    args_t *nxt;
    FILE *fp;
    int status;
    int pipefd[2];
    int ofd, ifd;

    ifd = fileno(stdin);
    ofd = fileno(stdout);

    for(int i = 0; i < count; i++)
    {
        cmd = args + i;
        nxt = cmd + 1;

        // stdout and stdin via files

        if(cmd->ofs)
        {
            fp = fopen(cmd->ofs, "w");
            if(!fp)
            {
                printf("%s: %s\n", cmd->ofs, strerror(errno));
                break;
            }
            cmd->ofd = fileno(fp);
        }

        if(cmd->ifs)
        {
            fp = fopen(cmd->ifs, "r");
            if(!fp)
            {
                printf("%s: %s\n", cmd->ifs, strerror(errno));
                break;
            }
            cmd->ifd = fileno(fp);
        }

        // execute command

        if(cmd->sep == 2)
        {
            status = run_command(cmd);
            if(status)
            {
                break;
            }
        }
        else if(cmd->sep == 3)
        {
            if(pipe(pipefd) < 0)
            {
                printf("failed to create pipe\n");
                break;
            }

            cmd->wait = 0;
            cmd->ofd = pipefd[1];
            run_command(cmd);
            nxt->ifd = pipefd[0];
        }
        else if(cmd->sep == 4)
        {
            status = run_command(cmd);
            if(!status)
            {
                break;
            }
        }
        else if(cmd->sep == 5)
        {
            run_command(cmd);
        }

        // close file descriptors early when possible

        if(cmd->ofd != ofd)
        {
            close(cmd->ofd);
            cmd->ofd = ofd;
        }

        if(cmd->ifd != ifd)
        {
            close(cmd->ifd);
            cmd->ifd = ifd;
        }
    }

    // close any remaining file descriptors (e.g. from breaking the loop)
    // wait for background children to finish

    for(int i = 0; i < count; i++)
    {
        cmd = args + i;
        if(cmd->ofd != ofd)
        {
            close(cmd->ofd);
        }
        if(cmd->ifd != ifd)
        {
            close(cmd->ifd);
        }
        if(!cmd->wait)
        {
            wait(cmd->pid, &status);
        }
    }

    return 0;
}

int main(int argc, char *argv[])
{
    char cmdline[128];
    int count;

    setpwd();

    hist_max = 100;
    hist_pos = 0;
    hist_len = 0;
    hist = calloc(hist_max, sizeof(char*));

    while(1)
    {
        print_shell();
        fflush(stdout);

        if(yash_gets(cmdline) == NULL)
        {
            break;
        }

        count = parse_cmdline(cmdline, args);
        if(count <= 0)
        {
            if(count < 0)
            {
                printf("invalid command: %s\n", cmdline);
            }
            continue;
        }

        execute(count, args);
    }

    return 0;
}
