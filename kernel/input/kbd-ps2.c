#include <kernel/input/input.h>
#include <kernel/input/ps2.h>
#include <kernel/mem/heap.h>
#include <kernel/x86/irq.h>

static uint8_t kbd_def_keys[] = {
    0,              // 0x00
    KEY_F9,         // 0x01
    0,              // 0x02
    KEY_F5,         // 0x03
    KEY_F3,         // 0x04
    KEY_F1,         // 0x05
    KEY_F2,         // 0x06
    KEY_F12,        // 0x07
    0,              // 0x08
    KEY_F10,        // 0x09
    KEY_F8,         // 0x0a
    KEY_F6,         // 0x0b
    KEY_F4,         // 0x0c
    KEY_TAB,        // 0x0d
    KEY_GRAVE,      // 0x0e
    0,              // 0x0f
    0,              // 0x10
    KEY_LEFTALT,    // 0x11
    KEY_LEFTSHIFT,  // 0x12
    0,              // 0x13
    KEY_LEFTCTRL,   // 0x14
    KEY_Q,          // 0x15
    KEY_1,          // 0x16
    0,              // 0x17
    0,              // 0x18
    0,              // 0x19
    KEY_Z,          // 0x1a
    KEY_S,          // 0x1b
    KEY_A,          // 0x1c
    KEY_W,          // 0x1d
    KEY_2,          // 0x1e
    0,              // 0x1f
    0,              // 0x20
    KEY_C,          // 0x21
    KEY_X,          // 0x22
    KEY_D,          // 0x23
    KEY_E,          // 0x24
    KEY_4,          // 0x25
    KEY_3,          // 0x26
    0,              // 0x27
    0,              // 0x28
    KEY_SPACE,      // 0x29
    KEY_V,          // 0x2a
    KEY_F,          // 0x2b
    KEY_T,          // 0x2c
    KEY_R,          // 0x2d
    KEY_5,          // 0x2e
    0,              // 0x2f
    0,              // 0x30
    KEY_N,          // 0x31
    KEY_B,          // 0x32
    KEY_H,          // 0x33
    KEY_G,          // 0x34
    KEY_Y,          // 0x35
    KEY_6,          // 0x36
    0,              // 0x37
    0,              // 0x38
    0,              // 0x39
    KEY_M,          // 0x3a
    KEY_J,          // 0x3b
    KEY_U,          // 0x3c
    KEY_7,          // 0x3d
    KEY_8,          // 0x3e
    0,              // 0x3f
    0,              // 0x40
    KEY_COMMA,      // 0x41
    KEY_K,          // 0x42
    KEY_I,          // 0x43
    KEY_O,          // 0x44
    KEY_0,          // 0x45
    KEY_9,          // 0x46
    0,              // 0x47
    0,              // 0x48
    KEY_DOT,        // 0x49
    KEY_SLASH,      // 0x4a
    KEY_L,          // 0x4b
    KEY_SEMICOLON,  // 0x4c
    KEY_P,          // 0x4d
    KEY_MINUS,      // 0x4e
    0,              // 0x4f
    0,              // 0x50
    0,              // 0x51
    KEY_APOSTROPHE, // 0x52
    0,              // 0x53
    KEY_LEFTBRACE,  // 0x54
    KEY_EQUAL,      // 0x55
    0,              // 0x56
    0,              // 0x57
    KEY_CAPSLOCK,   // 0x58
    KEY_RIGHTSHIFT, // 0x59
    KEY_ENTER,      // 0x5a
    KEY_RIGHTBRACE, // 0x5b
    0,              // 0x5c
    KEY_BACKSLASH,  // 0x5d
    0,              // 0x5e
    0,              // 0x5f
    0,              // 0x60
    0,              // 0x61
    0,              // 0x62
    0,              // 0x63
    0,              // 0x64
    0,              // 0x65
    KEY_BACKSPACE,  // 0x66
    0,              // 0x67
    0,              // 0x68
    KEY_KP1,        // 0x69
    0,              // 0x6a
    KEY_KP4,        // 0x6b
    KEY_KP7,        // 0x6c
    0,              // 0x6d
    0,              // 0x6e
    0,              // 0x6f
    KEY_KP0,        // 0x70
    KEY_KPDOT,      // 0x71
    KEY_KP2,        // 0x72
    KEY_KP5,        // 0x73
    KEY_KP6,        // 0x74
    KEY_KP8,        // 0x75
    KEY_ESC,        // 0x76
    KEY_NUMLOCK,    // 0x77
    KEY_F11,        // 0x78
    KEY_KPPLUS,     // 0x79
    KEY_KP3,        // 0x7a
    KEY_KPMINUS,    // 0x7b
    KEY_KPASTERISK, // 0x7c
    KEY_KP9,        // 0x7d
    KEY_SCROLLLOCK, // 0x7e
    0,              // 0x7f
};

static uint8_t kbd_ext_keys[] = {
    0,              // 0x00
    0,              // 0x01
    0,              // 0x02
    0,              // 0x03
    0,              // 0x04
    0,              // 0x05
    0,              // 0x06
    0,              // 0x07
    0,              // 0x08
    0,              // 0x09
    0,              // 0x0a
    0,              // 0x0b
    0,              // 0x0c
    0,              // 0x0d
    0,              // 0x0e
    0,              // 0x0f
    0,              // 0x10
    KEY_RIGHTALT,   // 0x11
    0,              // 0x12
    0,              // 0x13
    KEY_RIGHTCTRL,  // 0x14
    0,              // 0x15
    0,              // 0x16
    0,              // 0x17
    0,              // 0x18
    0,              // 0x19
    0,              // 0x1a
    0,              // 0x1b
    0,              // 0x1c
    0,              // 0x1d
    0,              // 0x1e
    KEY_LEFTMETA,   // 0x1f
    0,              // 0x20
    0,              // 0x21
    0,              // 0x22
    0,              // 0x23
    0,              // 0x24
    0,              // 0x25
    0,              // 0x26
    KEY_RIGHTMETA,  // 0x27
    0,              // 0x28
    0,              // 0x29
    0,              // 0x2a
    0,              // 0x2b
    0,              // 0x2c
    0,              // 0x2d
    0,              // 0x2e
    0,              // 0x2f
    0,              // 0x30
    0,              // 0x31
    0,              // 0x32
    0,              // 0x33
    0,              // 0x34
    0,              // 0x35
    0,              // 0x36
    0,              // 0x37
    0,              // 0x38
    0,              // 0x39
    0,              // 0x3a
    0,              // 0x3b
    0,              // 0x3c
    0,              // 0x3d
    0,              // 0x3e
    0,              // 0x3f
    0,              // 0x40
    0,              // 0x41
    0,              // 0x42
    0,              // 0x43
    0,              // 0x44
    0,              // 0x45
    0,              // 0x46
    0,              // 0x47
    0,              // 0x48
    0,              // 0x49
    KEY_KPSLASH,    // 0x4a
    0,              // 0x4b
    0,              // 0x4c
    0,              // 0x4d
    0,              // 0x4e
    0,              // 0x4f
    0,              // 0x50
    0,              // 0x51
    0,              // 0x52
    0,              // 0x53
    0,              // 0x54
    0,              // 0x55
    0,              // 0x56
    0,              // 0x57
    0,              // 0x58
    0,              // 0x59
    KEY_KPENTER,    // 0x5a
    0,              // 0x5b
    0,              // 0x5c
    0,              // 0x5d
    0,              // 0x5e
    0,              // 0x5f
    0,              // 0x60
    0,              // 0x61
    0,              // 0x62
    0,              // 0x63
    0,              // 0x64
    0,              // 0x65
    0,              // 0x66
    0,              // 0x67
    0,              // 0x68
    KEY_END,        // 0x69
    0,              // 0x6a
    KEY_LEFT,       // 0x6b
    KEY_HOME,       // 0x6c
    0,              // 0x6d
    0,              // 0x6e
    0,              // 0x6f
    KEY_INSERT,     // 0x70
    KEY_DELETE,     // 0x71
    KEY_DOWN,       // 0x72
    0,              // 0x73
    KEY_RIGHT,      // 0x74
    KEY_UP,         // 0x75
    0,              // 0x76
    0,              // 0x77
    0,              // 0x78
    0,              // 0x79
    KEY_PAGEDOWN,   // 0x7a
    0,              // 0x7b
    0,              // 0x7c
    KEY_PAGEUP,     // 0x7d
    0,              // 0x7e
    0,              // 0x7f
};

static void kbd_handle_set_two(ps2_kbd_t *kbd, uint8_t code)
{
    int key;

    kbd->history = ((kbd->history << 8) | code);
    key = 0;

    if(code == 0xE1)
    {
        kbd->special = 1;
    }
    else if(code == 0xE0)
    {
        kbd->extended = 1;
    }
    else if(code == 0xF0)
    {
        kbd->released = 1;
    }
    else
    {
        if(kbd->extended)
        {
            key = kbd_ext_keys[code];
        }
        else
        {
            key = kbd_def_keys[code];
        }
    }

    if(kbd->special)
    {
        if((kbd->history & 0xFFFFFF) == 0xE11477)
        {
            kbd->released = 0;
            kbd->special = 0;
            key = KEY_PAUSE;
        }
        else if((kbd->history & 0xFFFFFFFFFF) == 0xE1F014F077)
        {
            kbd->released = 1;
            kbd->special = 0;
            key = KEY_PAUSE;
        }
    }
    else
    {
        if((kbd->history & 0xFFFFFFFF) == 0xE012E07C)
        {
            kbd->released = 0;
            key = KEY_SYSRQ;
        }
        else if((kbd->history & 0xFFFFFFFFFFFF) == 0xE0F07CE0F012)
        {
            kbd->released = 1;
            key = KEY_SYSRQ;
        }
    }

    if(kbd->special == 0)
    {
        if(key)
        {
            input_kbd_write(key, kbd->released);
            kbd->extended = 0;
            kbd->released = 0;
        }
    }
}

static void kbd_set_led(void *data, int status)
{
    ps2_port_t *dev = data;
    ps2_device_write(dev->port, 0xED);
    ps2_device_write(dev->port, status);
}

static void kbd_handler(int gsi, void *data)
{
    ps2_kbd_t *kbd;
    int value;

    kbd = data;
    value = ps2_device_read();

    if(kbd->scan_code_set == 1)
    {
        // not implemented
    }
    else if(kbd->scan_code_set == 2)
    {
        kbd_handle_set_two(kbd, value);
    }
}

void ps2_kbd_init(ps2_port_t *dev)
{
    // keyboard_t *kbd;
    int set, ident;
    int vector;

    // Enable scanning
    ps2_device_write(dev->port, 0xF4);

    // Current scan code set
    ident = (dev->ident >> 8);
    if((ident == 0x41) || (ident == 0xC1))
    {
        set = 1;
    }
    else
    {
        ps2_device_write(dev->port, 0xF0);
        ps2_device_write(dev->port, 0x00);
        do {
            set = ps2_device_read(); // TODO: for some reason we receive several ACKs
        } while(set == 0xFA);
    }

    // Information
    dev->kbd.special = 0;
    dev->kbd.extended = 0;
    dev->kbd.released = 0;
    dev->kbd.history = 0;
    dev->kbd.scan_code_set = set;

    // Register handler
    vector = irq_alloc_gsi_vector(dev->gsi);
    irq_request(vector, kbd_handler, &dev->kbd);
}
