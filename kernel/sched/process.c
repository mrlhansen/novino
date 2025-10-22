#include <kernel/sched/scheduler.h>
#include <kernel/sched/process.h>
#include <kernel/mem/heap.h>
#include <kernel/mem/vmm.h>
#include <kernel/vfs/vfs.h>
#include <kernel/vfs/fd.h>
#include <kernel/sysinfo.h>
#include <kernel/errno.h>
#include <string.h>

static LIST_INIT(global_process_list, process_t, plink);

static pid_t next_pid()
{
    static pid_t pid = 0;
    return pid++;
}

process_t *process_handle()
{
    return scheduler_get_thread()->parent;
}

void process_append_thread(process_t *process, thread_t *thread)
{
    thread->tid = process->next_tid++;
    thread->parent = process;
    list_insert(&process->threads, thread);
}

void process_remove_thread(process_t *process, thread_t *thread) // TODO: process is redundant here, it can only be thread->parent
{
    list_remove(&process->threads, thread);
}

pid_t process_wait(pid_t pid, int *status)
{
    process_t *self;
    process_t *child;
    int found;

    self = process_handle();
    found = 0;

    acquire_lock(&self->wait.lock);

    child = self->children.head;
    while(child)
    {
        if(child->pid == pid)
        {
            found = 1;
        }

        if(child->state == TERMINATED)
        {
            if(pid == 0 || child->pid == pid)
            {
                break;
            }
        }

        child = child->sibling.next;
    }

    release_lock(&self->wait.lock);

    if(child)
    {
        list_remove(&self->children, child);
        list_remove(&global_process_list, child);

        *status  = child->exitcode;
        pid = child->pid;

        // kfree(process); -- race condition, child might still be running -- idle thread for cleaning this up

        return pid;
    }

    if(!found && pid)
    {
        return -EINVAL; // pid is not our child
    }

    // wait for child
    acquire_lock(&self->wait.lock);
    self->wait.thread = thread_handle();
    self->wait.pid = pid;
    thread_block(&self->wait.lock);

    // try again after unblock
    return process_wait(pid, status);
}

void process_exit(int status)
{
    process_t *self;
    process_t *child;
    process_t *parent;
    thread_t *thread;
    fd_t *fd;

    // TODO: we assume that the process only has a single running thread when process_exit is called

    self = process_handle();
    parent = self->parent;

    // close file descriptors
    while(fd = list_head(&self->fd.list), fd)
    {
        vfs_close(fd->id);
    }

    // close working directory
    vfs_proc_fini(self);

    // destroy address space
    vmm_destroy_user_space();

    // parent adopts children
    while(child = list_pop(&self->children), child)
    {
        child->parent = parent;
        list_append(&parent->children, child);
    }

    // notify parent
    acquire_lock(&parent->wait.lock);

    thread = 0;
    self->exitcode = status;
    self->state = TERMINATED;

    if(parent->wait.thread)
    {
        if(parent->wait.pid == 0 || parent->wait.pid == self->pid)
        {
            thread = parent->wait.thread;
            parent->wait.thread = 0;
        }
    }

    release_lock(&parent->wait.lock);

    if(thread)
    {
        thread_unblock(thread);
    }

    // exit thread
    thread_exit();
}

process_t *process_create(const char *name, uint64_t pml4, process_t *parent)
{
    process_t *process;

    // Allocate memory
    process = kzalloc(sizeof(process_t));
    if(process == 0)
    {
        return 0;
    }

    // Create new address space, if needed
    if(pml4 == 0)
    {
        pml4 = vmm_create_user_space();
        if(pml4 == 0)
        {
            kfree(process);
            return 0;
        }
    }

    // Fill structure
    strscpy(process->name, name, sizeof(process->name));
    process->pid = next_pid();
    process->next_tid = 0;
    process->state = RUNNING;
    process->uid = 0;
    process->gid = 0;
    process->cwd = 0;
    process->pml4 = pml4;
    process->fd.next = 0;

    list_init(&process->children, offsetof(process_t, sibling));
    list_init(&process->threads, offsetof(thread_t, plink));
    list_init(&process->fd.list, offsetof(fd_t, link));

    // Handle parent
    if(parent)
    {
        process->uid = parent->uid;
        process->gid = parent->gid;
        process->cwd = parent->cwd;
        process->parent = parent;
        list_insert(&parent->children, process);
    }
    else
    {
        process->parent = process;
    }

    // Set working directory
    vfs_proc_init(process, process->cwd);

    // Append to list of processes
    list_insert(&global_process_list, process);

    // Return process
    return process;
}

void sysinfo_proclist(sysinfo_t *sys)
{
    process_t *item = 0;
    while(item = list_iterate_reverse(&global_process_list, item), item)
    {
        sysinfo_write(sys, "%d", item->pid);
    }
}

void sysinfo_procinfo(sysinfo_t *sys, size_t pid)
{
    process_t *item = 0;
    thread_t *th = 0;

    while(item = list_iterate(&global_process_list, item), item)
    {
        if(item->pid == pid)
        {
            break;
        }
    }

    if(!item)
    {
        return;
    }

    sysinfo_write(sys, "pid=%d", item->pid);
    sysinfo_write(sys, "ppid=%d", item->parent->pid);
    sysinfo_write(sys, "name=%s", item->name);
    sysinfo_write(sys, "state=%u", item->state);
    sysinfo_write(sys, "uid=%u", item->state);
    sysinfo_write(sys, "gid=%u", item->state);
    sysinfo_write(sys, "memsz=%u", item->brk.end - item->brk.start); // TODO: this is only brk memory, we also have mmap and the size of the program itself
    sysinfo_write(sys, "threads=%u", item->threads.length);

    while(th = list_iterate_reverse(&item->threads, th), th)
    {
        sysinfo_write(sys, "tid=%d", th->tid);
        sysinfo_write(sys, "name=%s", th->name);
        sysinfo_write(sys, "priority=%d", th->priority);
        sysinfo_write(sys, "state=%d", th->state);
        sysinfo_write(sys, "cputime=%lu", th->time_used);
    }
}
