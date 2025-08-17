#include <kernel/input/input.h>

void input_init()
{
    input_mouse_init();
    input_kbd_init();
}
