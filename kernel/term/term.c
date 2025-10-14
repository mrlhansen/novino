#include <kernel/sched/kthreads.h>
#include <kernel/input/input.h>
#include <kernel/term/term.h>
#include <kernel/vfs/vfs.h>
#include <kernel/debug.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

// VT = 0 (F1) kernel console (debug messages, read-only)
// VT = 1 (F2) normal terminal
// VT = 2 (F3) normal terminal
// VT = 3 (F4) normal terminal
// VT = 4 (F5) normal terminal
// VT = 5 (F6) normal terminal
// VT = 6 (F7) normal terminal
// VT = 7 (F8) graphical interface

// Active VT
static int vtnum = 0;
static vts_t *vts = 0;

// Alt
static int alt = 0;
static int lalt = 0;
static int ralt = 0;

// Control
static int ctrl = 0;
static int lctrl = 0;
static int rctrl = 0;

// Shift
static int shift = 0;
static int lshift = 0;
static int rshift = 0;

// Capslock
static int capslock = 0;
// numlock?

// Maps
static char *map_key_1[] = {"1", "1", "!", "!"};
static char *map_key_2[] = {"2", "2", "@", "@"};
static char *map_key_3[] = {"3", "3", "#", "#"};
static char *map_key_4[] = {"4", "4", "$", "$"};
static char *map_key_5[] = {"5", "5", "%", "%"};
static char *map_key_6[] = {"6", "6", "^", "^"};
static char *map_key_7[] = {"7", "7", "&", "&"};
static char *map_key_8[] = {"8", "8", "*", "*"};
static char *map_key_9[] = {"9", "9", "(", "("};
static char *map_key_0[] = {"0", "0", ")", ")"};
static char *map_key_a[] = {"a", "A", "A", "a"};
static char *map_key_b[] = {"b", "B", "B", "b"};
static char *map_key_c[] = {"c", "C", "C", "c"};
static char *map_key_d[] = {"d", "D", "D", "d"};
static char *map_key_e[] = {"e", "E", "E", "e"};
static char *map_key_f[] = {"f", "F", "F", "f"};
static char *map_key_g[] = {"g", "G", "G", "g"};
static char *map_key_h[] = {"h", "H", "H", "h"};
static char *map_key_i[] = {"i", "I", "I", "i"};
static char *map_key_j[] = {"j", "J", "J", "j"};
static char *map_key_k[] = {"k", "K", "K", "k"};
static char *map_key_l[] = {"l", "L", "L", "l"};
static char *map_key_m[] = {"m", "M", "M", "m"};
static char *map_key_n[] = {"n", "N", "N", "n"};
static char *map_key_o[] = {"o", "O", "O", "o"};
static char *map_key_p[] = {"p", "P", "P", "p"};
static char *map_key_q[] = {"q", "Q", "Q", "q"};
static char *map_key_r[] = {"r", "R", "R", "r"};
static char *map_key_s[] = {"s", "S", "S", "s"};
static char *map_key_t[] = {"t", "T", "T", "t"};
static char *map_key_u[] = {"u", "U", "U", "u"};
static char *map_key_v[] = {"v", "V", "V", "v"};
static char *map_key_w[] = {"w", "W", "W", "w"};
static char *map_key_x[] = {"x", "X", "X", "x"};
static char *map_key_y[] = {"y", "Y", "Y", "y"};
static char *map_key_z[] = {"z", "Z", "Z", "z"};

static char *map_key_grave[]      = {"`", "`", "~", "~"};
static char *map_key_minus[]      = {"-", "-", "_", "_"};
static char *map_key_equal[]      = {"=", "=", "+", "+"};
static char *map_key_leftbrace[]  = {"[", "[", "{", "{"};
static char *map_key_rightbrace[] = {"]", "]", "}", "}"};
static char *map_key_backslash[]  = {"\\", "\\", "|", "|"};
static char *map_key_semicolon[]  = {";", ";", ":", ":"};
static char *map_key_apostrophe[] = {"\"", "\"", "'", "'"};
static char *map_key_comma[]      = {",", ",", "<", "<"};
static char *map_key_dot[]        = {".", ".", ">", ">"};
static char *map_key_slash[]      = {"/", "/", "?", "?"};

static inline int tablen(char *buf, int size)
{
    int len = 0;
    for(int i = 0; i < size; i++)
    {
        switch(buf[i])
        {
            case '\e':
                len += 2;
                break;
            case '\t':
                len += 8 - (len % 8);
                break;
            default:
                len++;
                break;
        }
    }
    return 8 - (len % 8);
}

static void escape_color(int val, int *fg, int *bg)
{
    const int color_normal[] = {
        VGA_BLACK,
        VGA_RED,
        VGA_GREEN,
        VGA_BROWN,
        VGA_BLUE,
        VGA_MAGENTA,
        VGA_CYAN,
        VGA_WHITE,
    };

    const int color_bright[] = {
        VGA_GRAY,
        VGA_BRIGHT_RED,
        VGA_BRIGHT_GREEN,
        VGA_YELLOW,
        VGA_BRIGHT_BLUE,
        VGA_BRIGHT_MAGENTA,
        VGA_BRIGHT_CYAN,
        VGA_BRIGHT_WHITE,
    };

    if(val == 0)
    {
        *fg = VGA_BRIGHT_WHITE;
        *bg = VGA_BLACK;
    }
    else if(val == 39)
    {
        *fg = VGA_BRIGHT_WHITE;
    }
    else if(val == 49)
    {
        *bg = VGA_BLACK;
    }
    else if(val >= 30 && val <= 37)
    {
        *fg = color_normal[val-30];
    }
    else if(val >= 90 && val <= 97)
    {
        *fg = color_bright[val-90];
    }
    else if(val >= 40 && val <= 47)
    {
        *bg = color_normal[val-40];
    }
    else if(val >= 100 && val <= 107)
    {
        *bg = color_bright[val-100];
    }
}

static void handle_ansi_escape(vts_t *vts, int ch, int a, int b)
{
    if(ch == 'J') // ED = Erase in Display
    {
        if(a == 2)
        {
            console_clear(vts->console);
        }
    }
    else if(ch == 'A') // CUU = Cursor Up
    {
        a = max(a, 1);
        console_move_cursor(vts->console, 0, -a, true);
    }
    else if(ch == 'B') // CUD = Cursor Down
    {
        a = max(a, 1);
        console_move_cursor(vts->console, 0, a, true);
    }
    else if(ch == 'C') // CUF = Cursor Forward
    {
        a = max(a, 1);
        console_move_cursor(vts->console, a, 0, true);
    }
    else if(ch == 'D') // CUD = Cursor Back
    {
        a = max(a, 1);
        console_move_cursor(vts->console, -a, 0, true);
    }
    else if(ch == 'E') // CNL = Cursor Next Line
    {
        a = max(a, 1);
        console_move_cursor(vts->console, 0, a, true);
    }
    else if(ch == 'F') // CPL = Cursor Previous Line
    {
        a = max(a, 1);
        console_move_cursor(vts->console, 0, -a, true);
    }
    else if(ch == 'H') // CUP = Cursor Position
    {
        a = max(a, 1);
        b = max(b, 1);
        console_move_cursor(vts->console, b-1, a-1, false);
    }
    else if(ch == 'm') // SGR = Select Graphic Rendition
    {
        int fg = -1;
        int bg = -1;

        if(a >= 0)
        {
            escape_color(a, &fg, &bg);
        }

        if(b >= 0)
        {
            escape_color(b, &fg, &bg);
        }

        console_set_color(vts->console, fg, bg);
    }
}

void term_stdout(vts_t *vts, char *seq, size_t len)
{
    int csi = 0;
    int esc = 0;
    int ch, a, b;

    for(int i = 0; i < len; i++)
    {
        ch = seq[i];

        if(csi)
        {
            if(ch >= 0x40 && ch <= 0x7E)
            {
                if(b < 0)
                {
                    handle_ansi_escape(vts, ch, a, b);
                }
                else
                {
                    handle_ansi_escape(vts, ch, b, a);
                }
                csi = 0;
            }
            else
            {
                if(ch == ';')
                {
                    b = a;
                    a = 0;
                }
                else
                {
                    a = 10*a + ctoi(ch);
                }
            }
        }
        else if(esc && ch == '[')
        {
            csi = 1;
            esc = 0;
            a = 0;
            b = -1;
        }
        else if(ch == '\e')
        {
            esc = 1;
        }
        else
        {
            console_putch(vts->console, ch);
            esc = 0;
        }
    }
}

static void term_stdin(char *seq)
{
    char *buf = vts->ibuf;
    int flags = vts->flags;
    int len = strlen(seq);
    int n = vts->ipos;
    int tab;

    // Use +2 to make room for \n\0 at the end
    if(n + len + 2 > sizeof(vts->ibuf))
    {
        if(seq[0] != '\n' && seq[0] != '\b')
        {
            return;
        }
    }

    // Copy sequence
    for(int i = 0; i < len; i++, n++)
    {
        buf[n] = seq[i];

        if(flags & ECHO)
        {
            if(seq[i] == '\e')
            {
                console_puts(vts->console, "^[");
            }
            else if(seq[i] == '\t')
            {
                tab = tablen(buf, n);
                while(tab--)
                {
                    console_putch(vts->console, ' ');
                }
            }
            else if(seq[i] == '\b')
            {
                // Editing is only allowed when buffering is enabled
                if(flags & NOBUF)
                {
                    continue;
                }

                // Delete backspace character
                buf[n--] = '\0';
                if(n < 0)
                {
                    continue;
                }

                // Delete previous character
                if(buf[n] == '\e')
                {
                    console_puts(vts->console, "\b\b  \b\b");
                }
                else if(buf[n] == '\t')
                {
                    tab = tablen(buf, n);
                    while(tab--)
                    {
                        console_putch(vts->console, '\b');
                    }
                }
                else
                {
                    console_puts(vts->console, "\b \b"); // We should work in runes for utf8
                }
                buf[n--] = '\0';
            }
            else if(seq[i] == '\n')
            {
                console_putch(vts->console, '\n');
            }
            else if(seq[i] >= 0x20)
            {
                console_putch(vts->console, seq[i]);
            }
        }
        else if(flags & ECHONL)
        {
            if(seq[i] == '\n')
            {
                console_putch(vts->console, '\n');
            }
        }
    }

    vts->ipos = n;

    if((flags & NOBUF) || (seq[0] == '\n'))
    {
        vts_flush_input(vts);
    }
}

static void term_kbd_input(input_event_t *ev)
{
    int done;

    // Modifier keys
    done = 1;
    switch(ev->code)
    {
        case KEY_LEFTALT:
            lalt = !ev->value;
            break;
        case KEY_RIGHTALT:
            ralt = !ev->value;
            break;
        case KEY_LEFTCTRL:
            lctrl = !ev->value;
            break;
        case KEY_RIGHTCTRL:
            rctrl = !ev->value;
            break;
        case KEY_LEFTSHIFT:
            lshift = !ev->value;
            break;
        case KEY_RIGHTSHIFT:
            rshift = !ev->value;
            break;
        case KEY_CAPSLOCK:
            if(ev->value == 0)
            {
                capslock ^= 1;
            }
            break;
        default:
            done = 0;
            break;
    }

    if(done)
    {
        alt = (lalt || ralt);
        ctrl = (lctrl || rctrl);
        shift = (lshift || rshift);
        return;
    }

    // Key releases
    if(ev->value == 1)
    {
        return;
    }

    // Terminal switch
    if(ctrl && alt)
    {
        switch(ev->code)
        {
            case KEY_F1:
                term_switch(0);
                break;
            case KEY_F2:
                term_switch(1);
                break;
            case KEY_F3:
                term_switch(2);
                break;
            case KEY_F4:
                term_switch(3);
                break;
            case KEY_F5:
                term_switch(4);
                break;
            case KEY_F6:
                term_switch(5);
                break;
            case KEY_F7:
                term_switch(6);
                break;
        }
        return;
    }

    // Kernel console is read-only
    if(vtnum == 0)
    {
        return;
    }

    // Terminal signals
    if(ctrl && !alt)
    {
        switch(ev->code)
        {
            case KEY_C: // sigint
                console_puts(vts->console, "^C");
                break;
            case KEY_D: // eof on stdin
                vts_flush_input(vts);
                break;
            case KEY_Z: // sigtstp
                console_puts(vts->console, "^Z");
                break;
        }
        return;
    }

    // Special keys (modifiers are missing)
    done = 1;
    switch(ev->code)
    {
        case KEY_ESC:
            term_stdin("\e");
            break;
        case KEY_UP:
            term_stdin("\e[A");
            break;
        case KEY_DOWN:
            term_stdin("\e[B");
            break;
        case KEY_RIGHT:
            term_stdin("\e[C");
            break;
        case KEY_LEFT:
            term_stdin("\e[D");
            break;
        case KEY_HOME:
            term_stdin("\e[H");
            break;
        case KEY_END:
            term_stdin("\e[F");
            break;
        case KEY_INSERT:
            term_stdin("\e[2~");
            break;
        case KEY_DELETE:
            term_stdin("\e[3~");
            break;
        case KEY_F1:
            term_stdin("\e[1P");
            break;
        case KEY_F2:
            term_stdin("\e[1Q");
            break;
        case KEY_F3:
            term_stdin("\e[1R");
            break;
        case KEY_F4:
            term_stdin("\e[1S");
            break;
        case KEY_PAGEUP:
            term_stdin("\e[5~");
            break;
        case KEY_PAGEDOWN:
            term_stdin("\e[6~");
            break;
        case KEY_BACKSPACE:
            term_stdin("\b");
            break;
        case KEY_TAB:
            term_stdin("\t");
            break;
        case KEY_ENTER:
            term_stdin("\n");
            break;
        case KEY_SPACE:
            term_stdin(" ");
            break;
        default:
            done = 0;
            break;
    }

    if(done)
    {
        return;
    }

    // Numpad (we ignore numlock for now)
    done = 1;
    switch (ev->code)
    {
        case KEY_KP0:
            term_stdin("0");
            break;
        case KEY_KP1:
            term_stdin("1");
            break;
        case KEY_KP2:
            term_stdin("2");
            break;
        case KEY_KP3:
            term_stdin("3");
            break;
        case KEY_KP4:
            term_stdin("4");
            break;
        case KEY_KP5:
            term_stdin("5");
            break;
        case KEY_KP6:
            term_stdin("6");
            break;
        case KEY_KP7:
            term_stdin("7");
            break;
        case KEY_KP8:
            term_stdin("8");
            break;
        case KEY_KP9:
            term_stdin("9");
            break;
        case KEY_KPSLASH:
            term_stdin("/");
            break;
        case KEY_KPASTERISK:
            term_stdin("*");
            break;
        case KEY_KPMINUS:
            term_stdin("-");
            break;
        case KEY_KPPLUS:
            term_stdin("+");
            break;
        case KEY_KPENTER:
            term_stdin("\n");
            break;
        case KEY_KPDOT:
            term_stdin(".");
            break;
        default:
            done = 0;
            break;
    }

    if(done)
    {
        return;
    }

    // 0b000 = 0 = none
    // 0b001 = 1 = caps
    // 0b010 = 2 = shift
    // 0b011 = 3 = shift + caps
    // 0b100 = 4 = alt
    // 0b101 = 5 = alt + caps
    // 0b111 = 6 = alt + shift + caps

    int index = capslock + 2*shift; // + 4*alt
    char *ch = 0;
    switch(ev->code)
    {
        case KEY_0:
            ch = map_key_0[index];
            break;
        case KEY_1:
            ch = map_key_1[index];
            break;
        case KEY_2:
            ch = map_key_2[index];
            break;
        case KEY_3:
            ch = map_key_3[index];
            break;
        case KEY_4:
            ch = map_key_4[index];
            break;
        case KEY_5:
            ch = map_key_5[index];
            break;
        case KEY_6:
            ch = map_key_6[index];
            break;
        case KEY_7:
            ch = map_key_7[index];
            break;
        case KEY_8:
            ch = map_key_8[index];
            break;
        case KEY_9:
            ch = map_key_9[index];
            break;
        case KEY_A:
            ch = map_key_a[index];
            break;
        case KEY_B:
            ch = map_key_b[index];
            break;
        case KEY_C:
            ch = map_key_c[index];
            break;
        case KEY_D:
            ch = map_key_d[index];
            break;
        case KEY_E:
            ch = map_key_e[index];
            break;
        case KEY_F:
            ch = map_key_f[index];
            break;
        case KEY_G:
            ch = map_key_g[index];
            break;
        case KEY_H:
            ch = map_key_h[index];
            break;
        case KEY_I:
            ch = map_key_i[index];
            break;
        case KEY_J:
            ch = map_key_j[index];
            break;
        case KEY_K:
            ch = map_key_k[index];
            break;
        case KEY_L:
            ch = map_key_l[index];
            break;
        case KEY_M:
            ch = map_key_m[index];
            break;
        case KEY_N:
            ch = map_key_n[index];
            break;
        case KEY_O:
            ch = map_key_o[index];
            break;
        case KEY_P:
            ch = map_key_p[index];
            break;
        case KEY_Q:
            ch = map_key_q[index];
            break;
        case KEY_R:
            ch = map_key_r[index];
            break;
        case KEY_S:
            ch = map_key_s[index];
            break;
        case KEY_T:
            ch = map_key_t[index];
            break;
        case KEY_U:
            ch = map_key_u[index];
            break;
        case KEY_V:
            ch = map_key_v[index];
            break;
        case KEY_W:
            ch = map_key_w[index];
            break;
        case KEY_X:
            ch = map_key_x[index];
            break;
        case KEY_Y:
            ch = map_key_y[index];
            break;
        case KEY_Z:
            ch = map_key_z[index];
            break;
        case KEY_GRAVE:
            ch = map_key_grave[index];
            break;
        case KEY_MINUS:
            ch = map_key_minus[index];
            break;
        case KEY_EQUAL:
            ch = map_key_equal[index];
            break;
        case KEY_LEFTBRACE:
            ch = map_key_leftbrace[index];
            break;
        case KEY_RIGHTBRACE:
            ch = map_key_rightbrace[index];
            break;
        case KEY_BACKSLASH:
            ch = map_key_backslash[index];
            break;
        case KEY_SEMICOLON:
            ch = map_key_semicolon[index];
            break;
        case KEY_APOSTROPHE:
            ch = map_key_apostrophe[index];
            break;
        case KEY_COMMA:
            ch = map_key_comma[index];
            break;
        case KEY_DOT:
            ch = map_key_dot[index];
            break;
        case KEY_SLASH:
            ch = map_key_slash[index];
            break;
        }

        if(ch)
        {
            term_stdin(ch);
        }
}

static void term_kbd_listen()
{
    input_event_t event;
    int fd;

    // Maybe this should hook directly into the input system to avoid the vfs?

    fd = vfs_open("/devices/keyboard", O_READ);
    while(1)
    {
        vfs_read(fd, sizeof(event), &event);
        term_kbd_input(&event);
    }
}

void term_switch(int num)
{
    if(num == vtnum)
    {
        return;
    }

    vts->console->active = 0;
    vts = vts_select(num);
    vts->console->active = 1;
    console_refresh(vts->console);

    vtnum = num;
}

void term_init()
{
    thread_t *wrk;

    vts_init();
    vts = vts_select(0);

    wrk = kthreads_create("term", term_kbd_listen, 0, TPR_HIGH);
    kthreads_run(wrk);
}
