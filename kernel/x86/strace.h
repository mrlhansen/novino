#pragma once

#include <kernel/types.h>

struct symtab {
    uint64_t address;
    uint32_t size;
    uint32_t length;
    char name[];
};

struct frame {
  struct frame *rbp;
  uint64_t rip;
};

void strace(int);
void strace_init(uint64_t, uint32_t);
