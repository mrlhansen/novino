#pragma once

#include <kernel/input/events.h>
#include <kernel/types.h>

typedef struct {
    uint64_t ts;
    uint16_t type;
    uint16_t code;
    uint32_t value;
} input_event_t;

void input_mouse_init();

void input_kbd_usb_boot_protocol(uint64_t, uint64_t);
void input_kbd_write(int, int);
void input_kbd_auto_repeat();
void input_kbd_init();


void input_init();
