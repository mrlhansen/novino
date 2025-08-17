#include <kernel/x86/ioports.h>
#include <kernel/debug.h>
#include <kernel/mem/heap.h>
#include <kernel/mem/vmm.h>
#include <kernel/pci/pci.h>
#include <sabi/host.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

void *sabi_host_alloc(int count, int size)
{
    void *ptr = (void*)kmalloc(size*count);
    memset(ptr, 0, size*count);
    return ptr;
}

void sabi_host_free(void *ptr)
{
    kfree(ptr);
}

void sabi_host_debug(const char *file, int line, const char *fmt, ...)
{
    char obuf[256];
    va_list args;

    va_start(args, fmt);
    vsprintf(obuf, fmt, args);
    va_end(args);

    kp_debug("sabi", "%s:%d: %s", file, line, obuf);
}

void sabi_host_panic()
{
    while(1);
}

uint64_t sabi_host_map(uint64_t address)
{
    return vmm_phys_to_virt(address);
}

void sabi_host_sleep(uint64_t usec)
{

}

void sabi_host_pmio_read(uint16_t address, uint64_t *value, int width)
{
    switch(width)
    {
        case 32:
            *value = inportl(address);
            break;
        case 16:
            *value = inportw(address);
            break;
        case 8:
            *value = inportb(address);
            break;
        default:
            *value = 0;
            break;
    }
}

void sabi_host_pmio_write(uint16_t address, uint64_t value, int width)
{
    switch(width)
    {
        case 32:
            outportl(address, value);
            break;
        case 16:
            outportw(address, value);
            break;
        case 8:
            outportb(address, value);
            break;
        default:
            break;
    }
}

void sabi_host_pci_read(sabi_pci_t *adr, uint32_t reg, uint64_t *value, int width)
{
    pci_dev_t dev;

    dev.mmio = 0;
    dev.pmio = ((adr->bus << 8) | (adr->slot << 3) | adr->func);
    *value = 0;

    switch(width)
    {
        case 32:
            pci_read_dword(&dev, reg, (uint32_t*)value);
            break;
        case 16:
            pci_read_word(&dev, reg, (uint16_t*)value);
            break;
        case 8:
            pci_read_byte(&dev, reg, (uint8_t*)value);
            break;
        default:
            break;
    }
}

void sabi_host_pci_write(sabi_pci_t *adr, uint32_t reg, uint64_t value, int width)
{
    pci_dev_t dev;

    dev.mmio = 0;
    dev.pmio = ((adr->bus << 8) | (adr->slot << 3) | adr->func);

    switch(width)
    {
        case 32:
            pci_write_dword(&dev, reg, value);
            break;
        case 16:
            pci_write_word(&dev, reg, value);
            break;
        case 8:
            pci_write_byte(&dev, reg, value);
            break;
        default:
            break;
    }
}
