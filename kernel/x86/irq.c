#include <kernel/x86/ioapic.h>
#include <kernel/x86/lapic.h>
#include <kernel/x86/irq.h>
#include <kernel/x86/smp.h>
#include <kernel/x86/idt.h>
#include <kernel/x86/isr.h>
#include <kernel/acpi/acpi.h>
#include <kernel/mem/heap.h>
#include <kernel/debug.h>
#include <kernel/errno.h>
#include <stdlib.h>

// Vector 0-31 are exceptions
// Vector 32-47 are system reserved
// Vector 48-239 are used for IRQs
// Vector 240-255 are system reserved

static int gsi_max;
static int irq_max;
static gsi_t *gsi_data = 0;
static irq_t *irq_data = 0;
extern void (*irq_stubs[])();

static irq_t *get_irq_item(int vector)
{
    if(vector >= 48 && vector <= 239)
    {
        return irq_data + vector - 48;
    }
    return 0;
}

static gsi_t *get_gsi_item(int gsi)
{
    if(gsi >= 0 && gsi < gsi_max)
    {
        return gsi_data + gsi;
    }
    return 0;
}

static irq_t *irq_allocate(int num)
{
    irq_t *item;
    irq_t *irq = 0;
    int cnt = 0;

    for(int i = 0; i < irq_max; i++)
    {
        item = irq_data + i;

        if(!cnt)
        {
            if(item->vector % num)
            {
                continue;
            }
        }

        if(item->free)
        {
            if(!irq)
            {
                irq = item;
            }

            cnt++;

            if(cnt == num)
            {
                break;
            }
        }
        else
        {
            cnt = 0;
            irq = 0;
        }
    }

    if(cnt < num)
    {
        return 0;
    }

    for(int i = 0; i < cnt; i++)
    {
        irq[i].apic_id = smp_core_apic_id(irq[i].vector);
        irq[i].free = 0;
    }

    return irq;
}

void irq_handler(stack_t *stack)
{
    irq_handler_item_t *item;
    irq_t *irq;
    int vector;

    vector = 48 + stack->int_no;
    irq = get_irq_item(vector);
    item = irq->handlers.head;

    while(item)
    {
        item->handler(vector, item->data);
        item = item->link.next;
    }

    irq->count++;
    lapic_write(APIC_EOI, 0);
}

// Translate legacy ISA interrupt number to GSI number,
// based on interrupt source overrides in the ACPI tables
int irq_translate(int id)
{
    gsi_t *gsi;

    gsi = get_gsi_item(id);
    if(gsi)
    {
       return gsi->redirection;
    }

    return -ENOINT;
}

// Configure GSI interupt, based on information from the ACPI tables
void irq_configure(int source, int gsi, int polarity, int mode)
{
    gsi_data[source].redirection = gsi;
    gsi_data[gsi].polarity = polarity;
    gsi_data[gsi].mode = mode;
    gsi_data[gsi].present = 1;

    const char *map[] = {
        "active high",
        "active low",
        "edge-triggered",
        "level-triggered",
    };

    kp_info("irq", "gsi %02d (%s, %s)", gsi, map[polarity], map[2+mode]);
}

// Return the APIC ID of the CPU that handles this interrupt vector
int irq_affinity(int vector)
{
    irq_t *irq;

    irq = get_irq_item(vector);
    if(irq == 0)
    {
        return -EINVAL;
    }

    return irq->apic_id;
}

int irq_alloc_gsi_vector(int id)
{
    redir_t entry;
    gsi_t *gsi;
    irq_t *irq;

    // Get data
    gsi = get_gsi_item(id);
    if(gsi == 0)
    {
        return -ENXIO;
    }
    if(gsi->present == 0)
    {
        return -ENXIO;
    }

    // Vector already configured
    if(gsi->vector)
    {
        return gsi->vector;
    }

    // Allocate new vector
    irq = irq_allocate(1);
    if(irq == 0)
    {
        return -ENOSPC;
    }

    irq->gsi = gsi;
    gsi->vector = irq->vector;

    entry.vector = irq->vector;
    entry.delv_mode = FIXED;
    entry.dest_mode = 0;
    entry.delv_status = 0;
    entry.polarity = gsi->polarity;
    entry.remote_irr = 0;
    entry.mode = gsi->mode;
    entry.mask = 0;
    entry.reserved = 0;
    entry.destination = irq->apic_id;

    kp_debug("irq", "allocated vector %d with apic_id %d for gsi %d", irq->vector, irq->apic_id, gsi->id);

    idt_set_gate(irq->vector, irq->stub);
    ioapic_route(gsi->id, &entry);

    return irq->vector;
}

int irq_alloc_msi_vectors(int num)
{
    irq_t *irq;

    // must be powers of two
    if(num & (num - 1))
    {
        return -EINVAL;
    }

    irq = irq_allocate(num);
    if(irq == 0)
    {
        return -ENOSPC;
    }

    for(int i = 0; i < num; i++)
    {
        kp_debug("irq", "allocated vector %d with apic_id %d for msi", irq[i].vector, irq[i].apic_id);
        idt_set_gate(irq[i].vector, irq[i].stub);
    }

    return irq->vector;
}

int irq_free_vector(int vector)
{
    redir_t entry;
    irq_t *irq;

    // Validate arguments
    irq = get_irq_item(vector);
    if(irq == 0)
    {
        return -EINVAL;
    }

    if(irq->handlers.length > 0)
    {
        return -EBUSY;
    }

    // Free vector
    if(irq->gsi)
    {
        entry.vector = 0;
        entry.delv_mode = 0;
        entry.dest_mode = 0;
        entry.delv_status = 0;
        entry.polarity = 0;
        entry.remote_irr = 0;
        entry.mode = 0;
        entry.mask = 1;
        entry.reserved = 0;
        entry.destination = 0;

        ioapic_route(irq->gsi->id, &entry);
        irq->gsi->vector = 0;
        irq->gsi = 0;
    }

    idt_clear_gate(irq->vector);
    irq->free = 1;

    return 0;
}

int irq_request(int vector, irq_handler_t handler, void *data)
{
    irq_handler_item_t *item;
    list_t *list;
    irq_t *irq;

    // Validate arguments
    irq = get_irq_item(vector);
    if(irq == 0)
    {
        return -EINVAL;
    }

    if(irq->free)
    {
        return -EINVAL;
    }

    // Install handler
    item = kzalloc(sizeof(*item));
    if(item == 0)
    {
        return -ENOMEM;
    }

    item->handler = handler;
    item->data = data;

    list = &irq->handlers;
    list_append(list, item);

    return 0;
}

int irq_free(int vector, void *data)
{
    irq_handler_item_t *item;
    list_t *list;
    irq_t *irq;

    // Validate arguments
    irq = get_irq_item(vector);
    if(irq == 0)
    {
        return -EINVAL;
    }

    if(irq->free)
    {
        return -EINVAL;
    }

    if(data == 0)
    {
        return -EINVAL;
    }

    // Remove handler
    list = &irq->handlers;
    item = list->head;

    while(item)
    {
        if(item->data == data)
        {
            break;
        }
        item = item->link.next;
    }

    if(item == 0)
    {
        return -EFAIL;
    }

    list_remove(list, item);
    kfree(item);

    return 0;
}

void irq_init()
{
    // Allocate GSI array
    gsi_max = max(ioapic_max_gsi, 16);
    gsi_data = kcalloc(gsi_max, sizeof(*gsi_data));
    if(gsi_data == 0)
    {
        kp_panic("irq", "failed to allocate gsi array");
    }

    // Allocate IRQ array
    irq_max = 192;
    irq_data = kcalloc(irq_max, sizeof(*irq_data));
    if(irq_data == 0)
    {
        kp_panic("irq", "failed to allocate irq array");
    }

    // Reset all vectors
    for(int i = 0; i < irq_max; i++)
    {
        irq_data[i].vector = 48 + i;
        irq_data[i].free = 1;
        irq_data[i].apic_id = 0;
        irq_data[i].stub = irq_stubs[i];
        list_init(&irq_data[i].handlers, offsetof(irq_handler_item_t, link));
    }

    // Default state
    for(int i = 0; i < gsi_max; i++)
    {
        gsi_data[i].id = i;
        gsi_data[i].redirection = i;
    }

    // Default configuration for ISA interrupts
    for(int i = 0; i < 16; i++)
    {
        gsi_data[i].polarity = HIGH;
        gsi_data[i].mode = EDGE;
        gsi_data[i].present = 1;
    }

    // Handle ISA overrides
    acpi_report_iso();
}

void sti()
{
    asm volatile("sti");
}

void cli()
{
    asm volatile("cli");
}
