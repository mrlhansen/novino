#pragma once

#include <kernel/types.h>

typedef struct {
    uint8_t special;
    uint8_t extended;
    uint8_t released;
    uint64_t history;
    uint8_t scan_code_set;
} ps2_kbd_t;

typedef struct {
    int port;    // Port number (0 or 1)
    int exists;  // Port exists (boolean)
    int device;  // Device is connected (boolean)
    int ident;   // Device identifier
    int type;    // Device type
    int gsi;     // Interrupt pin
    union {
        ps2_kbd_t kbd;
    } u;
} ps2_port_t;

#define PS2_UNKNOWN  0x00
#define PS2_KEYBOARD 0x01
#define PS2_MOUSE    0x02

#define PS2_DATA    0x60
#define PS2_STATUS  0x64
#define PS2_COMMAND 0x64

#define PS2_SR_OUTPUT  0x01
#define PS2_SR_INPUT   0x02
#define PS2_SR_SYSTEM  0x04
#define PS2_SR_COMMAND 0x08
#define PS2_SR_TMOUT   0x40
#define PS2_SR_PARITY  0x80

#define PS2_TIMEOUT 500000000UL

void ps2_kbd_init(ps2_port_t*);

int ps2_device_write(int, uint8_t);
int ps2_device_read();
void ps2_init();
