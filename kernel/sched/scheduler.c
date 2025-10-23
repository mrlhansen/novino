#include <kernel/sched/scheduler.h>
#include <kernel/sched/kthreads.h>
#include <kernel/x86/ioports.h>
#include <kernel/time/time.h>
#include <kernel/x86/lapic.h>
#include <kernel/x86/smp.h>
#include <kernel/mem/heap.h>
#include <kernel/mem/pmm.h>
#include <kernel/mem/vmm.h>
#include <kernel/debug.h>
#include <string.h>

static int core_count = 0;
static scheduler_t *sdata = 0;

static void set_scheduler(scheduler_t *scheduler)
{
    asm volatile("movq %0, %%dr3" :: "r" (scheduler));
}

static scheduler_t *get_scheduler()
{
    scheduler_t *scheduler;
    asm volatile("movq %%dr3, %0" : "=r" (scheduler));
    return scheduler;
}

static void scheduler_set_thread(scheduler_t *scheduler, thread_t *thread)
{
    scheduler->thread_current = thread;
    scheduler->tss->rsp0 = thread->stack;
    scheduler->xstate = thread->xstate;
    write_msr(MSR_KERNEL_GS_BASE, (uint64_t)&thread->gs);
}

thread_t *scheduler_get_thread()
{
    scheduler_t *scheduler;
    asm volatile("movq %%dr3, %0" : "=r" (scheduler));
    return scheduler->thread_current;
}

void scheduler_mask()
{
    lapic_write(APIC_TPR, 0x20);
}

void scheduler_unmask()
{
    lapic_write(APIC_TPR, 0);
}

void scheduler_stop()
{
    // bit 16 is the mask bit
    // 0x20 = 32 is the scheduler interrupt vector
    lapic_write(APIC_LVTT, 0x10020);
}

void scheduler_yield()
{
    scheduler_stop();
    switch_task();
}

static void scheduler_preempt(scheduler_t *scheduler)
{
    lapic_write(APIC_ICR1, scheduler->apic_id << 24);
    lapic_write(APIC_ICR0, 0x04020);
}

static uint64_t scheduler_next(scheduler_t *scheduler)
{
    thread_t *thread;
    list_t *queue;
    uint64_t ticks;

    acquire_lock(&scheduler->lock);
    ticks = scheduler->frequency;

    if(scheduler->srt.length)
    {
        thread = list_pop(&scheduler->srt);
        ticks = 0;
    }
    else if(scheduler->count)
    {
        while(1)
        {
            queue = scheduler->std + scheduler->index;
            if(queue->length)
            {
                thread = list_pop(queue);
                scheduler->count--;
                break;
            }
            scheduler->index = (scheduler->index + 1) % 9;
        }
        ticks /= 100;
    }
    else if(scheduler->idle.length)
    {
        thread = list_pop(&scheduler->idle);
        ticks /= 100;
    }
    else
    {
        thread = scheduler->thread_idle;
        ticks = 0;
    }

    scheduler_set_thread(scheduler, thread);
    release_lock(&scheduler->lock);

    return ticks;
}

static uint64_t scheduler_elapsed_time(scheduler_t *scheduler)
{
    uint64_t old, new;
    uint64_t elapsed;

    old = scheduler->tsc0;
    new = system_timestamp();
    elapsed = new - old;
    scheduler->tsc0 = new;

    return elapsed;
}

static void scheduler_update_load(scheduler_t *scheduler)
{
    uint64_t old, new;
    uint64_t load, diff;

    new = system_timestamp();
    old = scheduler->tsc1;
    diff = new - old;

    if(diff > 100000000UL)
    {
        scheduler->tsc1 = new;
        old = scheduler->tsc2;
        new = scheduler->thread_idle->time_used;
        scheduler->tsc2 = new;

        load = (new - old) * 1000;
        load = (load / diff);
        scheduler->load = (1000 - load);
    }
}

static scheduler_t *scheduler_select(thread_t *thread)
{
    scheduler_t *s;
    thread_t *ct;
    uint32_t core = 0;
    uint32_t min = ~0;

    for(int i = 0; i < core_count; i++)
    {
        s = sdata + i;
        ct = s->thread_current;

        if(ct == 0)
        {
            continue; // Only happens when initializing the scheduler
        }
        else if((ct == thread) && (s->count == 0))
        {
            core = i;
            break;
        }
        else if(ct->priority == TPR_IDLE)
        {
            core = i;
            break;
        }
        else if(s->count < min)
        {
            core = i;
            min = s->count;
        }
    }

    return sdata + core;
}

void scheduler_append(thread_t *thread)
{
    int pc, pn, preempt, index;
    scheduler_t *scheduler;
    uint32_t flags;

    scheduler = scheduler_select(thread);
    acquire_safe_lock(&scheduler->lock, &flags);

    preempt = 0;
    pc = scheduler->thread_current->priority;
    pn = thread->priority;

    if(pn == TPR_SRT)
    {
        list_append(&scheduler->srt, thread);
        if(pc < pn)
        {
            preempt = 1;
        }
    }
    else if(pn == TPR_IDLE)
    {
        list_append(&scheduler->idle, thread);
    }
    else
    {
        index = (scheduler->index + thread->max_count) % 9,
        list_append(scheduler->std + index, thread);
        scheduler->count++;
        if(pc == TPR_IDLE)
        {
            preempt = 1;
        }
    }

    if(preempt)
    {
        scheduler_preempt(scheduler);
    }

    release_safe_lock(&scheduler->lock, &flags);
}

uint64_t schedule_handler(uint64_t rsp)
{
    scheduler_t *scheduler;
    uint64_t old_pml4;
    uint64_t new_pml4;
    uint64_t elapsed;
    uint64_t ticks;
    thread_t *thread;

    scheduler = get_scheduler();
    elapsed = scheduler_elapsed_time(scheduler);
    thread = scheduler->thread_current;

    if(thread)
    {
        thread->rsp = rsp;
        thread->time_used += elapsed;
        old_pml4 = thread->parent->pml4;

        if(thread == scheduler->thread_idle)
        {
            thread->state = SLEEPING;
        }
        else if(thread->state == RUNNING)
        {
            thread->state = READY;
            scheduler_append(thread);
        }
        else if(thread->state == BLOCKING)
        {
            if(thread->lock)
            {
                release_lock(thread->lock);
                thread->lock = 0;
            }
        }

        thread->yield = 0;
    }
    else
    {
        old_pml4 = 0;
    }

    scheduler_update_load(scheduler);
    ticks = scheduler_next(scheduler);
    thread = scheduler->thread_current;

    rsp = thread->rsp;
    thread->state = RUNNING;
    new_pml4 = thread->parent->pml4;

    if(old_pml4 != new_pml4)
    {
        vmm_load_pml4(new_pml4);
    }

    if(ticks)
    {
        lapic_timer_init(ticks, false);
    }

    lapic_write(APIC_EOI, 0);
    return rsp;
}

void scheduler_init_core(int count, int id, int apic_id, tss_t *tss)
{
    scheduler_t *scheduler;
    uint64_t rsp;
    int memsz;

    if(sdata == 0)
    {
        core_count = count;
        memsz = count * sizeof(scheduler_t);
        sdata = kzalloc(memsz);
    }

    rsp = pmm_alloc_frame();
    rsp = vmm_phys_to_virt(rsp);

    scheduler = sdata + id;
    scheduler->rsp = rsp + PAGE_SIZE;
    scheduler->xstate = rsp;
    scheduler->apic_id = apic_id;
    scheduler->frequency = lapic_timer_calibrate();
    scheduler->thread_idle = kthreads_create_idle();
    scheduler->tss = tss;

    list_init(&scheduler->idle, offsetof(thread_t, slink));
    list_init(&scheduler->srt, offsetof(thread_t, slink));
    for(int i = 0; i < 9; i++)
    {
        list_init(scheduler->std + i, offsetof(thread_t, slink));
    }

    set_scheduler(scheduler);
}

void scheduler_init()
{
    scheduler_t *scheduler;
    thread_t *thread;

    scheduler = get_scheduler();
    thread = kthreads_create("main", 0, 0, TPR_HIGH);
    thread->state = RUNNING;
    scheduler_set_thread(scheduler, thread);
    scheduler_elapsed_time(scheduler);

    lapic_bcast_ipi(32, true, true);
}
