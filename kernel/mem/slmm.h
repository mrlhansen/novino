#pragma once

#include <kernel/types.h>

void slmm_init(size_t start, size_t end);
void slmm_report_reserved(int ready);
size_t slmm_kmalloc(size_t size, int align);
