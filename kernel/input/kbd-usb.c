#include <kernel/input/input.h>
#include <kernel/debug.h>
#include <string.h>

// USB Keyboard Boot Protocol
static const uint8_t kbd_bp_keys[] = {
    0,              // 0x00
    0,              // 0x01 - Overflow
    0,              // 0x02
    0,              // 0x03
    KEY_A,          // 0x04
    KEY_B,          // 0x05
    KEY_C,          // 0x06
    KEY_D,          // 0x07
    KEY_E,          // 0x08
    KEY_F,          // 0x09
    KEY_G,          // 0x0a
    KEY_H,          // 0x0b
    KEY_I,          // 0x0c
    KEY_J,          // 0x0d
    KEY_K,          // 0x0e
    KEY_L,          // 0x0f
    KEY_M,          // 0x10
    KEY_N,          // 0x11
    KEY_O,          // 0x12
    KEY_P,          // 0x13
    KEY_Q,          // 0x14
    KEY_R,          // 0x15
    KEY_S,          // 0x16
    KEY_T,          // 0x17
    KEY_U,          // 0x18
    KEY_V,          // 0x19
    KEY_W,          // 0x1a
    KEY_X,          // 0x1b
    KEY_Y,          // 0x1c
    KEY_Z,          // 0x1d
    KEY_1,          // 0x1e
    KEY_2,          // 0x1f
    KEY_3,          // 0x20
    KEY_4,          // 0x21
    KEY_5,          // 0x22
    KEY_6,          // 0x23
    KEY_7,          // 0x24
    KEY_8,          // 0x25
    KEY_9,          // 0x26
    KEY_0,          // 0x27
    KEY_ENTER,      // 0x28
    KEY_ESC,        // 0x29
    KEY_BACKSPACE,  // 0x2a
    KEY_TAB,        // 0x2b
    KEY_SPACE,      // 0x2c
    KEY_MINUS,      // 0x2d
    KEY_EQUAL,      // 0x2e
    KEY_LEFTBRACE,  // 0x2f
    KEY_RIGHTBRACE, // 0x30
    KEY_BACKSLASH,  // 0x31
    0,              // 0x32
    KEY_SEMICOLON,  // 0x33
    KEY_APOSTROPHE, // 0x34
    KEY_GRAVE,      // 0x35
    KEY_COMMA,      // 0x36
    KEY_DOT,        // 0x37
    KEY_SLASH,      // 0x38
    KEY_CAPSLOCK,   // 0x39
    KEY_F1,         // 0x3a
    KEY_F2,         // 0x3b
    KEY_F3,         // 0x3c
    KEY_F4,         // 0x3d
    KEY_F5,         // 0x3e
    KEY_F6,         // 0x3f
    KEY_F7,         // 0x40
    KEY_F8,         // 0x41
    KEY_F9,         // 0x42
    KEY_F10,        // 0x43
    KEY_F11,        // 0x44
    KEY_F12,        // 0x45
    KEY_SYSRQ,      // 0x46
    KEY_SCROLLLOCK, // 0x47
    KEY_PAUSE,      // 0x48
    KEY_INSERT,     // 0x49
    KEY_HOME,       // 0x4a
    KEY_PAGEUP,     // 0x4b
    KEY_DELETE,     // 0x4c
    KEY_END,        // 0x4d
    KEY_PAGEDOWN,   // 0x4e
    KEY_RIGHT,      // 0x4f
    KEY_LEFT,       // 0x50
    KEY_DOWN,       // 0x51
    KEY_UP,         // 0x52
    KEY_NUMLOCK,    // 0x53
    KEY_KPSLASH,    // 0x54
    KEY_KPASTERISK, // 0x55
    KEY_KPMINUS,    // 0x56
    KEY_KPPLUS,     // 0x57
    KEY_KPENTER,    // 0x58
    KEY_KP1,        // 0x59
    KEY_KP2,        // 0x5a
    KEY_KP3,        // 0x5b
    KEY_KP4,        // 0x5c
    KEY_KP5,        // 0x5d
    KEY_KP6,        // 0x5e
    KEY_KP7,        // 0x5f
    KEY_KP8,        // 0x60
    KEY_KP9,        // 0x61
    KEY_KP0,        // 0x62
    KEY_KPDOT,      // 0x63
    KEY_102ND,      // 0x64
    KEY_COMPOSE,    // 0x65
};

static const uint8_t kbd_bp_meta[] = {
    KEY_LEFTCTRL,   // Bit 0
    KEY_LEFTSHIFT,  // Bit 1
    KEY_LEFTALT,    // Bit 2
    KEY_LEFTMETA,   // Bit 3
    KEY_RIGHTCTRL,  // Bit 4
    KEY_RIGHTSHIFT, // Bit 5
    KEY_RIGHTALT,   // Bit 6
    KEY_RIGHTMETA,  // Bit 7
};

void input_kbd_usb_boot_protocol(uint64_t curr, uint64_t prev)
{
    uint8_t *ac = (void*)&curr;
    uint8_t *ap = (void*)&prev;
    int a;

    // Meta keys
    for(int i = 0; i < 8; i++)
    {
        a = (curr & 1) - (prev & 1);
        if(a < 0)
        {
            input_kbd_write(kbd_bp_meta[i], 1);
        }
        else if (a > 0)
        {
            input_kbd_write(kbd_bp_meta[i], 0);
        }

        curr >>= 1;
        prev >>= 1;
    }

    // Reserved
    curr >>= 8;
    prev >>= 8;

    // Pressed keys
    for(int i = 0; i < 6; i++)
    {
        a = ac[i];
        if(a == 0)
        {
            break;
        }

        if(memchr(ap, a, 6) == 0)
        {
            a = kbd_bp_keys[a];
            if(a > 0)
            {
                input_kbd_write(a, 0);
            }
            else
            {
                kp_debug("kbd", "unknown key pressed: %d", ac[i]);
            }
        }
    }

    // Released keys
    for(int i = 0; i < 6; i++)
    {
        a = ap[i];
        if(a == 0)
        {
            break;
        }

        if(memchr(ac, a, 6) == 0)
        {
            a = kbd_bp_keys[a];
            if(a > 0)
            {
                input_kbd_write(a, 1);
            }
        }
    }
}
