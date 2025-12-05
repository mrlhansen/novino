#include <kernel/input/input.h>
#include <kernel/input/ps2.h>
#include <kernel/x86/irq.h>

static void mouse_handler(int gsi, void *data)
{
    ps2_mouse_t *mouse;
    int value;

    mouse = data;
    value = ps2_device_read();

    switch(mouse->cycle)
    {
        case 0:
            mouse->prev = mouse->meta;
            mouse->meta = value;
            mouse->cycle++;
            break;
        case 1:
            mouse->relx = value;
            mouse->cycle++;
            break;
        case 2:
            mouse->rely = value;
            mouse->cycle = 0;
            break;
        default:
            break;
    }

    if(mouse->cycle)
    {
        return;
    }

    int relx   = mouse->relx - ((mouse->meta << 4) & 0x100);
    int rely   = mouse->rely - ((mouse->meta << 3) & 0x100);
    int left   = (mouse->meta & 1) - (mouse->prev & 1);
    int right  = (mouse->meta & 2) - (mouse->prev & 2);
    int middle = (mouse->meta & 4) - (mouse->prev & 4);

    if(relx)
    {
        input_mouse_write(EV_REL, REL_X, relx);
    }

    if(rely)
    {
        input_mouse_write(EV_REL, REL_Y, rely);
    }

    if(left)
    {
        input_mouse_write(EV_BTN, BTN_LEFT, left < 0);
    }

    if(right)
    {
        input_mouse_write(EV_BTN, BTN_RIGHT, right < 0);
    }

    if(middle)
    {
        input_mouse_write(EV_BTN, BTN_LEFT, middle < 0);
    }
}

void ps2_mouse_init(ps2_port_t *dev)
{
    int vector;

    // Enable scanning
    ps2_device_write(dev->port, 0xF4);

    // Register handler
    vector = irq_alloc_gsi_vector(dev->gsi);
    irq_request(vector, mouse_handler, &dev->mouse);
}
