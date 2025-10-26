#pragma once

#include <kernel/pci/pci.h>
#include <kernel/usb/usb-core.h>
#include <kernel/usb/xhcispecs.h>

#define XHCI_MAX_SLOTS    32
#define XHCI_MAX_EVENTS   128
#define XHCI_MAX_COMMANDS 128

// xhci-command.c
int xhci_command_reset_device(xhci_t *xhci, int slot, uint64_t phys);
int xhci_command_evaluate_context(xhci_t *xhci, int slot, uint64_t phys);
int xhci_command_configure_endpoint(xhci_t *xhci, int slot, uint64_t phys);
int xhci_command_address_device(xhci_t *xhci, int slot, uint64_t phys);
int xhci_command_enable_slot(xhci_t *xhci);
int xhci_command_disable_slot(xhci_t *xhci, int slot);
int xhci_command_noop(xhci_t *xhci);

void xhci_transfer_control(usb_dev_t*, usb_request_t*, uint64_t, int, int);
void xhci_transfer_normal(usb_dev_t*, int, uint64_t, int);

// xhci-endpoint.c
int xhci_endpoint_max_packet_size(usb_dev_t*);
int xhci_endpoint_interval(usb_dev_t*, usb_endpoint_descriptor_t*);
int xhci_endpoint_max_esit(usb_dev_t*, usb_endpoint_descriptor_t*, int, int);
int xhci_endpoint_configure(usb_dev_t*, usb_endpoint_descriptor_t*);
void xhci_endpoint_set_handler(usb_dev_t*, int, xhci_handler_t, void*);

// xhci-hub.c
void xhci_hub_register(xhci_t*);

// xhci.c
xhci_t *usb_dev_xhci(usb_dev_t*);
xhci_context_t *usb_dev_xhci_context(usb_dev_t*);

int xhci_create_command_ring(xhci_ring_t*);
void xhci_init_device(usb_dev_t*);
void xhci_fini_device(usb_dev_t*);
void xhci_init(pci_dev_t *pcidev, uint64_t address);
