#include <kernel/sched/scheduler.h>
#include <kernel/sched/threads.h>
#include <kernel/mem/heap.h>
#include <kernel/x86/smp.h>
#include <kernel/x86/fpu.h>
#include <kernel/debug.h>
#include <string.h>

static LIST_INIT(dead, thread_t, slink);
static LIST_INIT(hold, thread_t, slink);

thread_t *thread_create(const char *name, void *entry, void *arg0, void *arg1, void *arg2)
{
    thread_t *thread;
    uint64_t *stack;

    thread = kzalloc(sizeof(thread_t) + fpu_xstate_size() + STACK_SIZE);
    if(thread == 0)
    {
        return 0;
    }

    // stack address (required alignment is 16 bytes)
    thread->stack = (uint64_t)(thread + 1) + STACK_SIZE;
    thread->stack = (thread->stack & -16UL);

    stack = (uint64_t*)thread->stack;
    fpu_xstate_init(thread);

    *--stack = 0x10;              // SS
    *--stack = thread->stack;     // RSP
    *--stack = 0x0202;            // RFLAGS
    *--stack = 0x08;              // CS
    *--stack = (uint64_t)entry;   // RIP
    *--stack = 0;                 // RAX
    *--stack = 0;                 // RBX
    *--stack = 0;                 // RCX
    *--stack = (uint64_t)arg2;    // RDX
    *--stack = (uint64_t)arg0;    // RDI
    *--stack = (uint64_t)arg1;    // RSI
    *--stack = 0;                 // RBP
    *--stack = 0;                 // R8
    *--stack = 0;                 // R9
    *--stack = 0;                 // R10
    *--stack = 0;                 // R11
    *--stack = 0;                 // R12
    *--stack = 0;                 // R13
    *--stack = 0;                 // R14
    *--stack = 0;                 // R15

    strscpy(thread->name, name, sizeof(thread->name));
    thread->tid = 0;
    thread->rsp = (uint64_t)stack;
    thread->state = READY;
    thread->priority = 0;
    thread->parent = 0;
    thread->time_used = 0;
    thread->yield = 0;

    thread->gs.krsp = thread->stack;
    thread->gs.ursp = 0;

    thread->sig.recv = 0;
    thread->sig.wait = 0;
    thread->sig.lock = 0;

    return thread;
}

void thread_priority(thread_t *thread, priority_t priority)
{
    thread->priority = priority;
    thread->max_count = 0;

    if(priority == TPR_LOW)
    {
        thread->max_count = 8;
    }
    else if(priority == TPR_MID)
    {
        thread->max_count = 4;
    }
    else if(priority == TPR_HIGH)
    {
        thread->max_count = 2;
    }
}

void thread_exit()
{
    thread_t *thread;
    scheduler_stop();

    thread = thread_handle();
    thread->state = TERMINATED;
    process_remove_thread(thread);
    // do we need the dead queue? Could we not just free the memory in the schedule handler and that's it?
    list_append(&dead, thread); // we need something to free items in the dead queue

    scheduler_yield();
}

thread_t *thread_handle()
{
    return scheduler_get_thread();
}

void thread_block(spinlock_t *lock)
{
    thread_t *thread;
    scheduler_stop();

    thread = thread_handle();
    thread->lock = lock;
    thread->state = BLOCKING;
    list_append(&hold, thread);

    scheduler_yield();
}

void thread_unblock(thread_t *thread)
{
    scheduler_mask();

    if(list_remove(&hold, thread) == 0)
    {
        thread->state = READY;
        scheduler_append(thread);
    }

    scheduler_unmask();
}

void thread_signal(thread_t *thread)
{
    spinlock_t *lock;
    uint32_t flags;

    lock = &thread->sig.lock;
    acquire_safe_lock(lock, &flags);

    if(thread->sig.wait)
    {
        thread->sig.wait = 0;
        thread->sig.recv = 0;

        // there are 2 different scenarios when unblocking the thread
        // 1. yielding has already finished and we simply unblock the thread
        // 2. yielding is currently ongoing on another core, so we need to wait

        while(thread->yield)
        {
            asm("pause");
        }

        list_remove(&hold, thread);
        thread->state = READY;
        scheduler_append(thread);
    }
    else
    {
        thread->sig.recv = 1;
    }

    release_safe_lock(lock, &flags);
}

void thread_wait()
{
    spinlock_t *lock;
    thread_t *thread;
    uint32_t flags;

    thread = thread_handle();
    lock = &thread->sig.lock;
    acquire_safe_lock(lock, &flags);

    if(thread->sig.recv)
    {
        thread->sig.recv = 0;
        thread->sig.wait = 0;
    }
    else
    {
        thread->yield = 1;
        thread->sig.wait = 1;
        thread->state = BLOCKING;
        list_append(&hold, thread);
    }

    release_lock(lock);

    if(thread->yield)
    {
        scheduler_yield();
    }

    restore_interrupts(&flags);
}
