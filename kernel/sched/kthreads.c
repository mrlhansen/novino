#include <kernel/sched/scheduler.h>
#include <kernel/sched/kthreads.h>
#include <kernel/mem/vmm.h>

static process_t *kernel;

static void kthreads_idle()
{
    while(1)
    {
        asm volatile("hlt");
    }
}

void kthreads_run(thread_t *thread)
{
    scheduler_append(thread);
}

thread_t *kthreads_create(const char *name, void *entry, void *args, priority_t priority)
{
    thread_t *thread;
    thread = thread_create(name, entry, args, 0, 0);
    thread_priority(thread, priority);
    process_append_thread(kernel, thread);
    return thread;
}

thread_t *kthreads_create_idle()
{
    thread_t *thread;

    thread = thread_create("idle", kthreads_idle, 0, 0, 0);
    thread->state = SLEEPING;
    thread_priority(thread, TPR_IDLE);
    process_append_thread(kernel, thread);

    return thread;
}

void kthreads_init()
{
    uint64_t pml4;
    pml4 = vmm_get_kernel_pml4();
    kernel = process_create("kernel", pml4, 0);
}
