#include <novino/termio.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "ted.h"

static void prompt_info(tui_t *tui, const char *text)
{
    static button_t btn[] = {
        [0] = {
            .text = "OK",
            .value = 'O',
            .active = 1
        }
    };

    static prompt_t p = {
        .text = 0,
        .action = INFO,
        .nbtn = 1,
        .btn = btn
    };

    p.text = text;
    tui->p = &p;
    tiosetflags(TIONOBUF | TIONOSCRL);
}


static void prompt_exit(tui_t *tui)
{
    static button_t btn[] = {
        [0] = {
            .text = "Yes",
            .value = 'Y',
            .active = 0
        },
        [1] = {
            .text = "No",
            .value = 'N',
            .active = 0
        },
        [2] = {
            .text = "Cancel",
            .value = 'C',
            .active = 1
        }
    };

    static prompt_t p = {
        .text = "Do you want to save the file before exiting?",
        .action = EXIT,
        .nbtn = 3,
        .btn = btn
    };

    tui->p = &p;
    tiosetflags(TIONOBUF | TIONOSCRL);
}

static void prompt_save(tui_t *tui)
{
    static button_t btn[] = {
        [0] = {
            .text = "Yes",
            .value = 'Y',
            .active = 1
        },
        [1] = {
            .text = "No",
            .value = 'N',
            .active = 0
        }
    };

    static prompt_t p = {
        .text = "Do you want to save the file?",
        .action = SAVE,
        .nbtn = 2,
        .btn = btn
    };

    tui->p = &p;
    tiosetflags(TIONOBUF | TIONOSCRL);
}

static void prompt_close(tui_t *tui, int *value, action_t *action)
{
    prompt_t *p = tui->p;
    button_t *b = p->btn;

    for(int i = 0; i < p->nbtn; i++)
    {
        if(b->active)
        {
            break;
        }
        b++;
    }

    *value = b->value;
    *action = p->action;

    tui->p = 0;
    tui->w.r.all = 1;
    tiosetflags(TIONOBUF | TIOCURSOR | TIONOSCRL);
    tui_refresh(tui);
}

static void save_file(tui_t *tui, const char *name, file_t *file)
{
    static char text[128];
    int status;

    status = file_write(name, file);
    if(status < 0)
    {
        sprintf(text, "Failed to save file: %s", strerror(errno));
        prompt_info(tui, text);
    }
}

int main(int argc, char *argv[])
{
    tiowinsz_t w;
    action_t action;
    file_t *file;
    char buf[64];
    char *seq;
    int value;
    int flags;
    int stop;
    int ch;

    if(argc != 2)
    {
        printf("Usage: %s [file]\n", argv[0]);
        return 0;
    }

    file = file_read(argv[1]);
    if(file == NULL)
    {
        printf("%s: %s: %s\n", argv[0], argv[1], strerror(errno));
        return -1;
    }

    tiogetwinsz(&w);
    tiogetflags(&flags);
    tiosetflags(TIONOBUF | TIOCURSOR | TIONOSCRL);

    tui_t tui = {
        .rows = w.rows,
        .cols = w.cols,
        .name = argv[1],
        .w = {
            .x = {
                .offset = 0,
                .size = w.cols,
                .pos = 0,
            },
            .y = {
                .offset = 1,
                .size = w.rows - 2,
                .pos = 0,
            },
            .wx = {
                .offset = 0,
                .pos = 0,
            },
            .wy = {
                .offset = 0,
                .pos = 0,
            },
            .r = {
                .all = 1,
                .line = 0,
            },
            .file = file,
        },
    };

    printf("\e[2J");
    stop = 0;

    while(!stop || tui.p)
    {
        tui_refresh(&tui);

        seq = tiogets(buf, sizeof(buf));
        if(seq == 0)
        {
            continue;
        }

        while(*seq)
        {
            ch = *seq++;

            if(ch == '\e')
            {
                int a, b;

                seq -= 1;
                seq += tiogetescape(seq, &ch, &a, &b);
                ch = (a << 8) | ch;

                if(tui.p)
                {
                    switch(ch)
                    {
                        case 0x0043: // C
                            prompt_right(tui.p);
                            break;
                        case 0x0044: // D
                            prompt_left(tui.p);
                            break;
                    }
                }
                else
                {
                    switch(ch)
                    {
                        case 0x0041: // A
                            textarea_up(&tui.w, 1);
                            break;
                        case 0x0042: // B
                            textarea_down(&tui.w, 1);
                            break;
                        case 0x0043: // C
                            textarea_right(&tui.w);
                            break;
                        case 0x0044: // D
                            textarea_left(&tui.w);
                            break;
                        case 0x0048: // H
                            textarea_home(&tui.w);
                            break;
                        case 0x0046: // F
                            textarea_end(&tui.w);
                            break;
                        case 0x0150: // 1P
                            prompt_save(&tui);
                            break;
                        case 0x0151: // 1Q
                            prompt_exit(&tui);
                            break;
                        case 0x037E: // 3~
                            // delete
                            break;
                        case 0x057E: // 5~
                            textarea_up(&tui.w, tui.w.y.size);
                            break;
                        case 0x067E: // 6~
                            textarea_down(&tui.w, tui.w.y.size);
                            break;
                    }
                }
            }
            else if(tui.p)
            {
                if(ch == '\n')
                {
                    prompt_close(&tui, &value, &action);
                    if(action == SAVE)
                    {
                        if(value == 'Y')
                        {
                            save_file(&tui, argv[1], file);
                        }
                    }
                    else if(action == EXIT)
                    {
                        if(value == 'Y')
                        {
                            save_file(&tui, argv[1], file);
                            stop = 1;
                        }
                        else if(value == 'N')
                        {
                            stop = 1;
                        }
                    }
                }
            }
            else if(ch == '\n')
            {
                textarea_enter(&tui.w);
            }
            else if(ch == '\b')
            {
                textarea_backspace(&tui.w);
            }
            else if(ch >= 32)
            {
                textarea_glyph(&tui.w, ch);
            }
        }
    }

    tiosetflags(flags);
    printf("\e[2J");
    fflush(stdout);

    return 0;
}
