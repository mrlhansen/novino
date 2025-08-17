#pragma once

#include <kernel/types.h>
#include <kernel/lists.h>

typedef void (*irq_handler_t)(int, void*);

typedef struct {
    uint8_t id;
    uint8_t polarity;
    uint8_t mode;
    uint8_t vector;
    uint8_t redirection;
    uint8_t present;
} gsi_t;

typedef struct {
    irq_handler_t handler;
    void *data;
    link_t link;
} irq_handler_item_t;

typedef struct {
    int vector;       // Vector number
    int free;         // Vector availability
    int apic_id;      // APIC ID of the CPU that receives this interrupt
    gsi_t *gsi;       // GSI for this vector (unset for MSI vectors)
    void (*stub)();   // Entrypoint for the IRQ handler
    size_t count;     // Number of times the interrupt has fired
    list_t handlers;  // List of installed handlers
} irq_t;

int irq_translate(int id);
int irq_affinity(int vector);
void irq_configure(int source, int gsi, int polarity, int mode);

int irq_alloc_gsi_vector(int id);
int irq_alloc_msi_vectors(int num);
int irq_free_vector(int vector);

int irq_request(int vector, irq_handler_t handler, void *data);
int irq_free(int vector, void *data);
void irq_init();

void sti();
void cli();
