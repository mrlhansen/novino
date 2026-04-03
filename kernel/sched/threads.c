#include <kernel/sched/scheduler.h>
#include <kernel/sched/process.h>
#include <kernel/mem/heap.h>
#include <kernel/x86/smp.h>
#include <kernel/x86/fpu.h>
#include <string.h>

static LIST_INIT(dead, thread_t, slink);

thread_t *thread_create(const char *name, void *entry, void *arg0, void *arg1, void *arg2)
{
    thread_t *thread;
    uint64_t rsp0;
    stack_t *stack;

    thread = kzalloc(sizeof(thread_t) + fpu_xstate_size() + STACK_SIZE);
    if(!thread)
    {
        return 0;
    }

    // stack address (required alignment is 16 bytes)
    rsp0 = (uint64_t)(thread + 1) + STACK_SIZE;
    rsp0 = (rsp0 & -16UL);

    // store stack address
    thread->rsp0 = rsp0;
    thread->rsp = rsp0 - sizeof(stack_t);
    stack = thread->stack;

    // initialize fpu state
    fpu_xstate_init(thread);

    // initialize stack
    stack->ss     = 0x10;
    stack->rsp    = thread->rsp0;
    stack->rflags = 0x0202;
    stack->cs     = 0x08;
    stack->rip    = (uint64_t)entry;
    stack->rax    = 0;
    stack->rbx    = 0;
    stack->rcx    = 0;
    stack->rdx    = (uint64_t)arg2;
    stack->rdi    = (uint64_t)arg0;
    stack->rsi    = (uint64_t)arg1;
    stack->rbp    = 0;
    stack->r8     = 0;
    stack->r9     = 0;
    stack->r10    = 0;
    stack->r11    = 0;
    stack->r12    = 0;
    stack->r13    = 0;
    stack->r14    = 0;
    stack->r15    = 0;

    // initialize the rest
    strscpy(thread->name, name, sizeof(thread->name));
    thread->state = READY;
    thread->gs.krsp = rsp0;
    thread->gs.ursp = 0;

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
    thread->yield = 1;
    list_append(&dead, thread);

    scheduler_yield();
}

thread_t *thread_handle()
{
    return scheduler_get_thread();
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
    }

    release_lock(lock);

    if(thread->yield)
    {
        scheduler_yield();
    }

    restore_interrupts(&flags);
}

void thread_idle_cleaning()
{
    static lock_t lock = 0;
    thread_t *item;

    if(atomic_lock(&lock))
    {
        return;
    }

    while(item = list_head(&dead), item)
    {
        if(item->yield)
        {
            break;
        }

        list_pop(&dead);
        process_remove_thread(item);

        // TODO:
        // When (item->signals & SIGTERM) and (this is the second-to-last thread), then
        // signal last thread if waiting in process_exit()

        kfree(item);
    }

    atomic_unlock(&lock);
}
