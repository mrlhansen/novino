#ifndef LOADER_BOOTSTRUCT_H
#define LOADER_BOOTSTRUCT_H

#include <stdint.h>

typedef struct {
    char signature[64];      // Bootloader signature
    char cmdline[256];       // Bootloader command line
    uint64_t lower_memory;   // Available lower memory
    uint64_t upper_memory;   // Available upper memory
    uint64_t initrd_address; // Address of init root module
    uint64_t initrd_size;    // Size of init root module
    uint64_t kernel_address; // Address of kernel code
    uint64_t kernel_size;    // Size of kernel in memory
    uint64_t mmap_address;   // Address of memory map
    uint64_t mmap_size;      // Size of memory map
    uint64_t symtab_address; // Address of symbol table
    uint64_t symtab_size;    // Size of symbol table
    uint64_t rsdp_address;   // Address of RSDP
    uint64_t rsdp_size;      // Size of RSDP
    uint64_t end_address;    // End of kernel memory region
    uint64_t lfb_address;    // Address of linear framebuffer
    uint64_t lfb_pitch;      // Pitch
    uint64_t lfb_width;      // Width
    uint64_t lfb_height;     // Height
    uint8_t lfb_mode;        // Mode (0 = text, 1 = graphics)
    uint8_t lfb_bpp;         // Bits per pixel
    uint8_t lfb_red_index;   // Red field position
    uint8_t lfb_red_mask;    // Red mask size
    uint8_t lfb_green_index; // Green field position
    uint8_t lfb_green_mask;  // Green mask size
    uint8_t lfb_blue_index;  // Blue field position
    uint8_t lfb_blue_mask;   // Blue mask size
} __attribute__((packed)) bootstruct_t;

#endif
