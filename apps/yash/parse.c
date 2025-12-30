#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "yash.h"

static char strbuf[256];

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
    args->ofd = STDOUT_FILENO;
    args->ifd = STDIN_FILENO;
    args->wait = 1;
}

int parse_cmdline(char *str, args_t *args)
{
    args_t *cmd;
    char *start;
    char *buf;

    int special = 0;
    int escape = 0;
    int quote = 0;
    int count = 0;
    int expect = 0;

    buf = strbuf;
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
