#include <kernel/time/time.h>
#include <kernel/x86/ioports.h>
#include <kernel/x86/irq.h>

void pit_init()
{
    uint8_t low, high;
    uint32_t divisor;
    int gsi, vector;

    // Register handler
    gsi = irq_translate(0);
    vector = irq_alloc_gsi_vector(gsi);
    irq_request(vector, timer_handler, 0);

    // Set frequency
    divisor = (1193182 / TIMER_FREQUENCY);
    low  = (divisor & 0xFF);
    high = ((divisor >> 8) & 0xFF);

    outportb(0x43, 0x36);
    outportb(0x40, low);
    outportb(0x40, high);
}
