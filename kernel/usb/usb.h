#pragma once

#include <kernel/usb/usb-core.h>

void *usb_next_descriptor(usb_dev_t*, void*);
void *find_endpoint_descriptor(usb_dev_t*, void*, int, int);
void *find_interface_descriptor(usb_dev_t*, void*);

void *usb_request_submit(usb_dev_t*, usb_request_t*, void*);
void usb_get_device_descriptor(usb_dev_t*, int);
void usb_get_string_descriptor(usb_dev_t*, int, char*);
void usb_get_config_descriptor(usb_dev_t*);
void usb_set_configuration(usb_dev_t*, int);
