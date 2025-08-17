#pragma once

#define DEFINE_AUTOFREE_TYPE(_type) \
    static inline void __free_##_type(_type **p) { if(*p) kfree(*p); *p = 0; }

#define autofree(_type) \
    __attribute__((cleanup(__free_##_type))) _type
