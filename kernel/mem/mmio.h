#pragma once

#include <kernel/types.h>

int mmio_alloc_uc_region(uint64_t, uint32_t, uint64_t*, uint64_t*);
int mmio_map_wc_region(uint64_t, uint64_t, uint64_t*);
