#pragma once

typedef unsigned long atomic_t;

static inline void atomic_set(atomic_t *var, atomic_t val)
{
    __atomic_store_n(var, val, __ATOMIC_RELAXED);
}

static inline atomic_t atomic_get(atomic_t *var)
{
    return __atomic_load_n(var, __ATOMIC_RELAXED);
}

static inline atomic_t atomic_add(atomic_t *var, atomic_t val)
{
    return __atomic_add_fetch(var, val, __ATOMIC_RELAXED);
}

static inline atomic_t atomic_sub(atomic_t *var, atomic_t val)
{
    return __atomic_sub_fetch(var, val, __ATOMIC_RELAXED);
}

static inline atomic_t atomic_inc(atomic_t *var)
{
    return __atomic_add_fetch(var, 1, __ATOMIC_RELAXED);
}

static inline atomic_t atomic_dec(atomic_t *var)
{
    return __atomic_sub_fetch(var, 1, __ATOMIC_RELAXED);
}
