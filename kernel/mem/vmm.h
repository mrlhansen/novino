#pragma once

#include <kernel/types.h>

#define ALIGN_TEST 0xFFF
#define ALIGN_MASK 0xFFFFFFFFFFFFF000
#define PAGE_SIZE  0x1000

#define LARGE_ALIGN_TEST 0x1FFFFF
#define LARGE_ALIGN_MASK 0xFFFFFFFFFFE00000
#define LARGE_PAGE_SIZE  0x200000

#define PAGE_ALIGN(a) \
    ((a + PAGE_SIZE - 1) & ALIGN_MASK)

#define LARGE_PAGE_ALIGN(a) \
    ((a + LARGE_PAGE_SIZE - 1) & LARGE_ALIGN_MASK)

#define KERNEL 0
#define USER   1
#define READ   0
#define WRITE  1

#define PAT_WB       0x00
#define PAT_WT       0x01
#define PAT_UC_MINUS 0x02
#define PAT_UC       0x03
#define PAT_WC       0x04
#define PAT_WP       0x05

#define PL4_BASE 0xFFFFFF7FBFDFE000
#define PL3_BASE 0xFFFFFF7FBFC00000
#define PL2_BASE 0xFFFFFF7F80000000
#define PL1_BASE 0xFFFFFF0000000000

#define AVL_ALLOCATED 1
#define AVL_MAPPED    2

#define USER_MMAP      0x200000000000 // 32 TiB
#define USER_MMAP_SIZE 0x600000000000 // 96 TiB

#define IDMAP      0xFFFF800000000000
#define IDMAP_SIZE 0x500000000000 // 80 TiB
#define HWMAP      0xFFFFD00000000000
#define HWMAP_SIZE 0x100000000000 // 16 TiB
#define KHEAP      0xFFFFE00000000000
#define KHEAP_SIZE 0x100000000000 // 16 TiB

typedef struct {
    uint32_t present   : 1;
    uint32_t write     : 1;
    uint32_t mode      : 1;
    uint32_t pwt       : 1;
    uint32_t pcd       : 1;
    uint32_t accessed  : 1;
    uint32_t zero      : 1;
    uint32_t ps        : 1;
    uint32_t avl       : 4;
    uint64_t next_base : 51;
    uint32_t nx        : 1;
} __attribute__((packed)) pde_t;

typedef struct {
    uint32_t present   : 1;
    uint32_t write     : 1;
    uint32_t mode      : 1;
    uint32_t pwt       : 1;
    uint32_t pcd       : 1;
    uint32_t accessed  : 1;
    uint32_t dirty     : 1;
    uint32_t ps        : 1;
    uint32_t global    : 1;
    uint32_t avl       : 3;
    uint32_t pat       : 1;
    uint32_t zero      : 8;
    uint64_t phys_addr : 42;
    uint32_t nx        : 1;
} __attribute__((packed)) pde2_t;

typedef struct {
    uint32_t present   : 1;
    uint32_t write     : 1;
    uint32_t mode      : 1;
    uint32_t pwt       : 1;
    uint32_t pcd       : 1;
    uint32_t accessed  : 1;
    uint32_t dirty     : 1;
    uint32_t pat       : 1;
    uint32_t global    : 1;
    uint32_t avl       : 3;
    uint64_t phys_addr : 51;
    uint32_t nx        : 1;
} __attribute__((packed)) pte_t;

int vmm_set_caching(uint64_t virt, uint8_t mode);
int vmm_set_mode(uint64_t virt, int w, int x);

int vmm_alloc_page(uint64_t virt);
int vmm_map_page(uint64_t virt, uint64_t phys);
int vmm_remap_page(uint64_t virt, uint64_t phys);
int vmm_free_page(uint64_t virt);

void vmm_load_pml4(uint64_t addr);
void vmm_load_kernel_pml4();
uint64_t vmm_get_kernel_pml4();
uint64_t vmm_get_current_pml4();

uint64_t vmm_create_user_space();
void vmm_destroy_user_space();

uint64_t vmm_phys_to_virt(uint64_t phys);
uint64_t vmm_virt_to_phys(uint64_t virt);

void vmm_init();
void pat_init();
