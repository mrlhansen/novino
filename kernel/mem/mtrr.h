#pragma once

#include <kernel/types.h>

#define MTRR_PHYS_BASE(n) (MTRR_PHYS_BASE0+2*(n))
#define MTRR_PHYS_MASK(n) (MTRR_PHYS_MASK0+2*(n))

enum {
    MEM_UC = 0,
    MEM_WC = 1,
    MEM_WT = 4,
    MEM_WP = 5,
    MEM_WB = 6,
};

enum {
    MTRR_CAP          = 0x0FE,
    SMRR_PHYS_BASE    = 0x1F2,
    SMRR_PHYS_MASK    = 0x1F3,
    MTRR_FIX64K_00000 = 0x250,
    MTRR_FIX16K_80000 = 0x258,
    MTRR_FIX16K_A0000 = 0x259,
    MTRR_FIX4K_C0000  = 0x268,
    MTRR_FIX4K_C8000  = 0x269,
    MTRR_FIX4K_D0000  = 0x26A,
    MTRR_FIX4K_D8000  = 0x26B,
    MTRR_FIX4K_E0000  = 0x26C,
    MTRR_FIX4K_E8000  = 0x26D,
    MTRR_FIX4K_F0000  = 0x26E,
    MTRR_FIX4K_F8000  = 0x26F,
    MTRR_PHYS_BASE0   = 0x200,
    MTRR_PHYS_MASK0   = 0x201,
    MTRR_DEF_TYPE     = 0x2FF,
};

typedef struct {
    uint8_t valid;
    uint8_t type;
    uint64_t base;
    uint64_t mask;
    uint64_t start;
    uint64_t length;
} mtrr_t;

void mtrr_init();
