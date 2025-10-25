#pragma once

typedef unsigned long atomic_t;
typedef unsigned char lock_t;

static inline void atomic_set(atomic_t *var, atomic_t val)
{
    __atomic_store_n(var, val, __ATOMIC_RELAXED);
}

static inline atomic_t atomic_get(atomic_t *var)
{
    return __atomic_load_n(var, __ATOMIC_RELAXED);
}

static inline atomic_t atomic_add_fetch(atomic_t *var, atomic_t val)
{
    return __atomic_add_fetch(var, val, __ATOMIC_RELAXED);
}

static inline atomic_t atomic_sub_fetch(atomic_t *var, atomic_t val)
{
    return __atomic_sub_fetch(var, val, __ATOMIC_RELAXED);
}

static inline atomic_t atomic_inc_fetch(atomic_t *var)
{
    return __atomic_add_fetch(var, 1, __ATOMIC_RELAXED);
}

static inline atomic_t atomic_dec_fetch(atomic_t *var)
{
    return __atomic_sub_fetch(var, 1, __ATOMIC_RELAXED);
}

static inline int atomic_lock(lock_t *lock)
{
    return __atomic_test_and_set(lock, __ATOMIC_ACQUIRE);
}

static inline int atomic_spinlock(lock_t *lock)
{
    while(__atomic_test_and_set(lock, __ATOMIC_ACQUIRE))
    {
        asm("pause");
    }
}

static inline void atomic_unlock(lock_t *lock)
{
    __atomic_clear(lock, __ATOMIC_RELEASE);
}
