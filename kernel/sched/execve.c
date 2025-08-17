#include <kernel/sched/scheduler.h>
#include <kernel/sched/execve.h>
#include <kernel/sched/elf.h>
#include <kernel/mem/heap.h>
#include <kernel/mem/vmm.h>
#include <kernel/mem/map.h>
#include <kernel/vfs/vfs.h>
#include <kernel/errno.h>
#include <kernel/debug.h> // tmp?
#include <string.h>

// TODO: There are so many places in this code where
// something will crash if we run out of memory

static size_t copy_argv(char **argv, uint64_t addr)
{
    int argc, memsz, offset;
    char **copy, *str;
    size_t end;

    argc = 0;
    memsz = 0;

    // Count number of arguments
    if(argv)
    {
        while(argv[argc])
        {
            memsz += 1 + strlen(argv[argc]);
            argc++;
        }
    }

    // Allocate memory
    offset = (argc + 1) * sizeof(void*);
    memsz = (memsz + offset);

    if(addr)
    {
        memsz = (memsz + PAGE_SIZE - 1) / PAGE_SIZE;
        end = addr;

        while(memsz--)
        {
            vmm_alloc_page(end);
            end += PAGE_SIZE;
        }
    }
    else
    {
        addr = (size_t)kzalloc(memsz);
        end = addr;
    }

    // Copy arguments
    copy = (void*)addr;
    str = (char*)(addr + offset);
    copy[argc] = 0;

    for(int i = 0; i < argc; i++)
    {
        strcpy(str, argv[i]);
        copy[i] = str;
        str += 1 + strlen(str);
    }

    return end;
}

static int execve_load(void *base, execve_t *opts)
{
    size_t brk, stack;
    size_t varg, venv;
    elf64_ehdr *ehdr;
    elf64_phdr *phdr;
    process_t *pr;
    uintptr_t rip;

    ehdr = (elf64_ehdr*)base;
    phdr = (elf64_phdr*)(base + ehdr->e_phoff);
    rip = ehdr->e_entry;
    brk = 0;

    for(int i = 0; i < ehdr->e_phnum; i++)
    {
        if(phdr->p_type == PT_LOAD)
        {
            size_t va, vb, msz;
            size_t fsz, x, w;
            int status;
            void *fp;

            // File
            fsz = phdr->p_filesz;
            fp = base + phdr->p_offset;

            // Memory size
            msz = phdr->p_memsz;
            if(msz & ALIGN_TEST)
            {
                msz &= ALIGN_MASK;
                msz += PAGE_SIZE;
            }

            // Memory address
            va = phdr->p_vaddr;
            vb = phdr->p_vaddr + msz;

            if(va & ALIGN_TEST)
            {
                return -ENOEXEC;
            }

            if(vb > brk)
            {
                brk = vb;
            }

            // Allocate pages
            for(size_t v = va; v < vb; v += PAGE_SIZE)
            {
                status = vmm_alloc_page(v);
                if(status < 0)
                {
                    return status;
                }
            }

            // Copy data
            memset((void*)va, 0, msz);
            memcpy((void*)va, fp, fsz);

            // Permissions
            w = (phdr->p_flags & PF_WRITE);
            x = (phdr->p_flags & PF_EXEC);

            for(size_t v = va; v < vb; v += PAGE_SIZE)
            {
                status = vmm_set_mode(v, w, x);
                if(status < 0)
                {
                    return status;
                }
            }
        }

        phdr++;
    }

    // Copy arguments
    varg = brk;
    venv = copy_argv(opts->argv, varg);
    brk  = copy_argv(opts->envp, venv);

    // Data segment
    pr = process_handle();
    pr->brk.start = brk;
    pr->brk.end = brk;
    pr->brk.max = USER_MMAP;

    // Create user space stack
    mm_init(&pr->mmap, USER_MMAP, USER_MMAP_SIZE);
    mm_map(&pr->mmap, PAGE_SIZE, MAP_STACK, &stack);

    // Clone file descriptors
    fd_create(opts->ifd);
    fd_create(opts->ofd);

    // Free kernel variables
    kfree(opts->argv);
    kfree(opts->envp);
    kfree(opts);

    // Execute
    switch_to_user_mode(rip, stack + PAGE_SIZE - 8, varg, venv); // TODO: The -8 fixes stack alignment!! Need to understand what GCC is doing here.
    return 0;
}

static void execve_stub(void *base, execve_t *opts)
{
    int status;

    status = execve_load(base, opts);
    kp_info("execve", "failed with status: %d", status);

    while(1);
}

pid_t execve(const char *filename, char **argv, char **envp, int stdin, int stdout)
{
    process_t *process;
    thread_t *thread;
    execve_t *opts;
    fd_t *ifd, *ofd;
    const char *name;
    elf64_ehdr *ehdr;
    int fd, status;
    stat_t stat;

    // File descriptors
    ifd = fd_find(stdin);
    ofd = fd_find(stdout);

    if((ifd == 0) || (ofd == 0))
    {
        return -EBADF;
    }

    // Open file
    fd = vfs_open(filename, O_READ);
    if(fd < 0)
    {
        return fd;
    }
    vfs_fstat(fd, &stat);

    // Allocate memory
    opts = kzalloc(sizeof(*opts) + stat.size);
    if(opts == 0)
    {
        vfs_close(fd);
        return -ENOMEM;
    }
    ehdr = (void*)(opts+1);

    // Read file
    status = vfs_read(fd, stat.size, ehdr);
    vfs_close(fd);

    if(status < 0)
    {
        kfree(ehdr);
        return status;
    }

    // Check ELF format
    const char magic[] = {
        0x7f, 'E', 'L', 'F', ELFCLASS64, ELFDATA2LSB, EV_CURRENT
    };

    if(memcmp(ehdr->e_ident, magic, sizeof(magic)) != 0)
    {
        return -ENOEXEC;
    }

    // Copy arguments
    opts->argv = (void*)copy_argv(argv, 0);
    opts->envp = (void*)copy_argv(envp, 0);
    opts->ifd = ifd->file;
    opts->ofd = ofd->file;

    // Process name
    name = strrchr(filename, '/');
    if(name == 0)
    {
        name = filename;
    }
    else
    {
        name++;
    }

    // Create new thread
    thread = thread_create(name, execve_stub, ehdr, opts, 0);
    thread_priority(thread, TPR_MID);

    // Create new process
    process = process_create(name, 0, process_handle());
    process_append_thread(process, thread);
    scheduler_append(thread);

    // Return PID of child
    return process->pid;
}
