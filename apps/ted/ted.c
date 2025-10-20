#include <novino/termio.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "ted.h"

int main(int argc, char *argv[])
{
    tiowinsz_t w;
    file_t *file;
    char buf[64];
    char *seq;
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

    while(!stop)
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
                        file_write(argv[1], file);
                        break;
                    case 0x0151: // 1Q
                        stop = 1;
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
