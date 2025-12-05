#include <kernel/input/input.h>

static const int mouse_bp_meta[] = {
    BTN_LEFT,   // Bit 0
    BTN_RIGHT,  // Bit 1
    BTN_MIDDLE, // Bit 2
};

void input_mouse_usb_boot_protocol(uint64_t curr, uint64_t prev)
{
    int8_t relx, rely;
    int a;

    relx = (curr >> 8) & 0xFF;
    rely = (curr >> 16) & 0xFF;

    for(int i = 0; i < 3; i++)
    {
        a = (curr & 1) - (prev & 1);
        if(a < 0)
        {
            input_mouse_write(EV_BTN, mouse_bp_meta[i], 1);
        }
        else if (a > 0)
        {
            input_mouse_write(EV_BTN, mouse_bp_meta[i], 0);
        }
        curr >>= 1;
        prev >>= 1;
    }

    if(relx)
    {
        input_mouse_write(EV_REL, REL_X, relx);
    }

    if(rely)
    {
        input_mouse_write(EV_REL, REL_Y, -rely);
    }
}
