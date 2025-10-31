#include <kernel/input/input.h>
#include <kernel/input/ps2.h>

void input_init()
{
    input_mouse_init();
    input_kbd_init();
    ps2_init();
}
