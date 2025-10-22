#pragma once

#include <kernel/types.h>

void pmm_free_frame(uint64_t);
void pmm_set_frame(uint64_t);

uint64_t pmm_alloc_frame();
uint64_t pmm_alloc_frames(uint32_t, uint32_t);

void pmm_set_available(uint64_t, uint64_t);
void pmm_set_reserved(uint64_t, uint64_t);

size_t pmm_usable_pages();
size_t pmm_free_pages();
void pmm_init();
