#pragma once

#include <kernel/input/events.h>
#include <kernel/types.h>

typedef struct {
    uint64_t ts;
    uint16_t type;
    uint16_t code;
    int32_t value;
} input_event_t;

void input_mouse_usb_boot_protocol(uint64_t curr, uint64_t prev);
void input_mouse_write(int type, int code, int value);
void input_mouse_init();

void input_kbd_usb_boot_protocol(uint64_t curr, uint64_t prev);
void input_kbd_write(int code, int value);
void input_kbd_auto_repeat();
void input_kbd_init();

void input_init();
