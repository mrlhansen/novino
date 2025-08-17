#include <kernel/x86/ioports.h>

void outportb(uint16_t port, uint8_t value)
{
    asm volatile("outb %0, %%dx" :: "a" (value), "d" (port));
}

uint8_t inportb(uint16_t port)
{
    uint8_t ret;
    asm volatile("inb %%dx, %0" : "=a" (ret) : "d" (port));
    return ret;
}

void outportw(uint16_t port, uint16_t value)
{
    asm volatile("outw %0, %%dx" :: "a" (value), "d" (port));
}

uint16_t inportw(uint16_t port)
{
    uint16_t ret;
    asm volatile("inw %%dx, %0" : "=a" (ret) : "d" (port));
    return ret;
}

void outportl(uint16_t port, uint32_t value)
{
    asm volatile("outl %0, %%dx" :: "a" (value), "d" (port));
}

uint32_t inportl(uint16_t port)
{
    uint32_t ret;
    asm volatile("inl %%dx, %0" : "=a" (ret) : "d" (port));
    return ret;
}

uint64_t read_msr(uint32_t msr)
{
    uint32_t hi, lo;
    asm volatile("rdmsr" : "=a" (lo), "=d" (hi) : "c" (msr));
    return ((uint64_t)hi << 32) | lo;
}

void write_msr(uint32_t msr, uint64_t val)
{
    uint32_t hi, lo;
    hi = (val >> 32);
    lo = (val & 0xFFFFFFFF);
    asm volatile("wrmsr" :: "a" (lo), "d" (hi), "c" (msr));
}
