#include <kernel/time/time.h>
#include <kernel/input/ps2.h>
#include <kernel/acpi/acpi.h>
#include <kernel/x86/ioports.h>
#include <kernel/x86/irq.h>
#include <kernel/x86/strace.h>
#include <kernel/errno.h>
#include <kernel/debug.h>

static ps2_port_t p0;
static ps2_port_t p1;

static void flush_data()
{
    uint8_t status;
    status = inportb(PS2_STATUS);

    while(status & PS2_SR_OUTPUT)
    {
        inportb(PS2_DATA);
        status = inportb(PS2_STATUS);
    }
}

static int read_data(uint8_t *value)
{
    uint8_t status;
    uint64_t tm, tmout;

    // Timeout counter
    tmout = system_timestamp();
    tmout = (tmout + PS2_TIMEOUT);

    // Poll until ready for reading
    do
    {
        status = inportb(PS2_STATUS);
        status &= PS2_SR_OUTPUT;
        tm = system_timestamp();
        if(tm > tmout)
        {
            return -ETMOUT;
        }
    }
    while(status == 0);

    // Read data
    *value = inportb(PS2_DATA);
    return 0;
}

static int write_data(uint8_t value)
{
    uint8_t status;
    uint64_t tm, tmout;

    // Timeout counter
    tmout = system_timestamp();
    tmout = (tmout + PS2_TIMEOUT);

    // Poll until ready for writing
    do
    {
        status = inportb(PS2_STATUS);
        status &= PS2_SR_INPUT;
        tm = system_timestamp();
        if(tm > tmout)
        {
            return -ETMOUT;
        }
    }
    while(status);

    // Write data
    outportb(PS2_DATA, value);
    return 0;
}

static int send_command(uint8_t value)
{
    uint8_t status;
    uint64_t tm, tmout;

    // Timeout counter
    tmout = system_timestamp();
    tmout = (tmout + PS2_TIMEOUT);

    // Poll until ready for writing
    do
    {
        status = inportb(PS2_STATUS);
        status &= PS2_SR_INPUT;
        tm = system_timestamp();
        if(tm > tmout)
        {
            return -ETMOUT;
        }
    }
    while(status);

    // Write command
    outportb(PS2_COMMAND, value);
    return 0;
}

static int read_config(uint8_t *value)
{
    int status;

    status = send_command(0x20);
    if(status < 0)
    {
        return status;
    }

    status = read_data(value);
    if(status < 0)
    {
        return status;
    }

    return 0;
}

static int write_config(uint8_t value)
{
    int status;

    status = send_command(0x60);
    if(status < 0)
    {
        return status;
    }

    status = write_data(value);
    if(status < 0)
    {
        return status;
    }

    return 0;
}

static int send_device_ack(int port, uint8_t value)
{
    int status;

    if(port)
    {
        status = send_command(0xD4);
        if(status < 0)
        {
            return status;
        }
    }

    status = write_data(value);
    if(status < 0)
    {
        return status;
    }

    status = read_data(&value);
    if(status < 0)
    {
        return status;
    }

    if(value != 0xFA)
    {
        return -ENOACK;
    }

    return 0;
}

static int reset_device(int port)
{
    uint8_t value;
    int status;

    status = send_device_ack(port, 0xFF);
    if(status < 0)
    {
        return status;
    }

    status = read_data(&value);
    if(status < 0)
    {
        return status;
    }

    if(value == 0xAA)
    {
        flush_data();
    }
    else
    {
        return -EFAIL;
    }

    return 0;
}

static int identify_device(int port, uint16_t *ident)
{
    int status;
    uint8_t *value;

    *ident = 0;
    value = (uint8_t*)ident;

    status = send_device_ack(port, 0xF5);
    if(status < 0)
    {
        return status;
    }

    status = send_device_ack(port, 0xF2);
    if(status < 0)
    {
        return status;
    }

    read_data(value);
    if(*value == 0xAB)
    {
        read_data(value+1);
    }

    return 0;
}

const char *decode_ident(ps2_port_t *dev)
{
    if(dev->ident == 0x0000)
    {
        dev->type = PS2_MOUSE;
        return "mouse (standard)";
    }
    else if(dev->ident == 0x0003)
    {
        dev->type = PS2_MOUSE;
        return "mouse (scroll wheel)";
    }
    else if(dev->ident == 0x0004)
    {
        dev->type = PS2_MOUSE;
        return "mouse (5-button)";
    }
    else if((dev->ident & 0xFF) == 0xAB)
    {
        dev->type = PS2_KEYBOARD;
        return "keyboard";
    }
    else
    {
        dev->type = PS2_UNKNOWN;
        return "unknown";
    }
}

int ps2_device_write(int port, uint8_t value)
{
    return send_device_ack(port, value);
}

int ps2_device_read()
{
    return inportb(PS2_DATA);
}

void ps2_init()
{
    uint8_t value, config;
    int dual, status;
    uint16_t devid;

    // First port default
    p0.port = 0;
    p0.exists = 0;
    p0.device = 0;
    p0.type = PS2_UNKNOWN;
    p0.gsi = irq_translate(1);

    // Second port default
    p1.port = 1;
    p1.exists = 0;
    p1.device = 0;
    p1.type = PS2_UNKNOWN;
    p1.gsi = irq_translate(12);

    // Check that the controller exists
    status = acpi_8042_support();
    if(status == 0)
    {
        return;
    }

    // Disable first port
    status = send_command(0xAD);
    if(status < 0)
    {
        return;
    }

    // Disable second port
    status = send_command(0xA7);
    if(status < 0)
    {
        return;
    }

    // Flush data buffer
    flush_data();

    // Read configuration
    read_config(&config);

    // If the second port was not disabled, then it does not exist
    if(config & 0x20)
    {
        dual = 1;
    }
    else
    {
        dual = 0;
    }

    // Disable interrupts and translation
    config = (config & 0xBC);
    write_config(config);

    // Perform self-test
    send_command(0xAA);
    read_data(&value);

    // Self-test failed
    if(value != 0x55)
    {
        return;
    }

    // In case of a reset
    write_config(config);
    flush_data();

    // Check that we actually have a dual device
    if(dual)
    {
        // Enable second port
        send_command(0xA8);

        // Read configuration
        read_config(&value);

        // Status of second port
        if(value & 0x20)
        {
            // Port was not enabled
            dual = 0;
        }
        else
        {
            // Disable port again
            send_command(0xA7);
        }
    }

    // Interface test (first port)
    send_command(0xAB);
    read_data(&value);
    if(value == 0)
    {
        p0.exists = 1;
    }

    // Interface test (second port)
    if(dual)
    {
        send_command(0xA9);
        read_data(&value);
        if(value == 0)
        {
            p1.exists = 1;
        }
    }

    // Enable first port
    if(p0.exists)
    {
        // Enable port
        send_command(0xAE);

        // Enable interrupts
        read_config(&config);
        write_config(config | 0x01);
    }

    // Enable second port
    if(p1.exists)
    {
        // Enable port
        send_command(0xA8);

        // Enable interrupts
        read_config(&config);
        write_config(config | 0x02);
    }

    // Reset device on first port
    if(p0.exists)
    {
        status = reset_device(0);
        if(status == 0)
        {
            status = identify_device(0, &devid);
            if(status == 0)
            {
                p0.device = 1;
                p0.ident = devid;
            }
        }
    }

    // Reset device on second port
    if(p1.exists)
    {
        status = reset_device(1);
        if(status == 0)
        {
            status = identify_device(1, &devid);
            if(status == 0)
            {
                p1.device = 1;
                p1.ident = devid;
            }
        }
    }

    // Print log info
    kp_info("ps2", "found 8042 controller, %d port(s)", p0.exists+p1.exists);

    if(p0.exists)
    {
        if(p0.device)
        {
            kp_info("ps2", "port0: %s [%04x], gsi: %d", decode_ident(&p0), p0.ident, p0.gsi);

        }
        else
        {
            kp_info("ps2", "port0: no device");
        }
    }

    if(p1.exists)
    {
        if(p1.device)
        {
            kp_info("ps2", "port1: %s [%04x], gsi: %d", decode_ident(&p1), p1.ident, p1.gsi);
        }
        else
        {
            kp_info("ps2", "port1: no device");
        }
    }

    // Initialize device drivers
    if(p0.type == PS2_KEYBOARD)
    {
        ps2_kbd_init(&p0);
    }

    if(p1.type == PS2_KEYBOARD)
    {
        ps2_kbd_init(&p1);
    }
}
