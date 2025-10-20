#include <kernel/syscalls/syscalls.h>
#include <kernel/sched/process.h>
#include <kernel/sched/threads.h>
#include <kernel/sched/execve.h>
#include <kernel/time/time.h>
#include <kernel/x86/ioports.h>
#include <kernel/mem/vmm.h>
#include <kernel/vfs/vfs.h>
#include <kernel/types.h>
#include <kernel/errno.h>
#include <kernel/debug.h>

#define assert_nonzero(ptr) \
    if(ptr == 0) return -EFAULT;

#define assert_userspace(ptr) \
    if((size_t)ptr > 0x00007FFFFFFFFFFF) return -EFAULT;

// assert that strings are null terminated? assert_string -> nonzero and strlen doesnt crash?

static long sys_default()
{
    return -ENOSYS;
}

static void sys_exit(int status)
{
    process_exit(status);
}

static pid_t sys_wait(pid_t pid, int *status)
{
    assert_nonzero(status);
    assert_userspace(status);
    return process_wait(pid, status);
}

static long sys_open(const char *path, int flags)
{
    assert_nonzero(path);
    assert_userspace(path);
    return vfs_open(path, flags);
}

static long sys_close(int fd)
{
    return vfs_close(fd);
}

static long sys_write(int fd, size_t size, void *buf)
{
    assert_nonzero(buf);
    assert_userspace(buf);
    return vfs_write(fd, size, buf);
}

static long sys_read(int fd, size_t size, void *buf)
{
    assert_nonzero(buf);
    assert_userspace(buf);
    return vfs_read(fd, size, buf);
}

static long sys_seek(int fd, long offset, int origin)
{
    return vfs_seek(fd, offset, origin);
}

static long sys_ioctl(int fd, size_t cmd, size_t val)
{
    return vfs_ioctl(fd, cmd, val);
}

static long sys_fstat(int fd, stat_t *stat)
{
    assert_nonzero(stat);
    assert_userspace(stat);
    return vfs_fstat(fd, stat);
}

static long sys_stat(const char *path, stat_t *stat)
{
    assert_nonzero(path);
    assert_userspace(path);
    assert_nonzero(stat);
    assert_userspace(stat);
    return vfs_stat(path, stat);
}

static long sys_readdir(int fd, size_t size, dirent_t *dirent)
{
    assert_nonzero(dirent);
    assert_userspace(dirent);
    return vfs_readdir(fd, size, dirent);
}

// returns new break point on success
// returns an error code on failure
static long sys_brk(size_t brk)
{
    size_t start, end, max;
    process_t *pr;
    int status;

    pr = process_handle();
    start = pr->brk.start;
    end = pr->brk.end;
    max = pr->brk.max;
    status = 0;

    if(brk == 0)
    {
        return end;
    }

    if(brk & ALIGN_TEST)
    {
        brk = (brk & ALIGN_MASK) + PAGE_SIZE;
    }

    if(brk == end)
    {
        return end;
    }

    if(brk < start)
    {
        return -EFAULT;
    }

    if(brk > max)
    {
        return -EFAULT;
    }

    if(brk > end)
    {
        while(end < brk)
        {
            if(vmm_alloc_page(end) < 0)
            {
                status = -ENOMEM;
                break;
            }
            end += PAGE_SIZE;
        }
    }
    else
    {
        while(end > brk)
        {
            end -= PAGE_SIZE;
            if(vmm_free_page(end) < 0)
            {
                status = -ENOMEM;
                break;
            }
        }
    }

    pr->brk.end = end;
    if(status < 0)
    {
        return status;
    }

    return end;
}

static pid_t sys_spawnve(const char *filename, char *argv[], char *envp[], int stdin, int stdout)
{
    int n;

    assert_nonzero(filename);
    assert_userspace(filename);

    // check memory position of all elements
    // check that the lists are zero terminated

    if(argv)
    {
        n = 0;
        assert_userspace(argv);
        while(argv[n])
        {
            assert_userspace(argv);
            n++;
        }
    }

    if(envp)
    {
        n = 0;
        assert_userspace(envp);
        while(envp[n])
        {
            assert_userspace(envp);
            n++;
        }
    }

    // exec program
    return execve(filename, argv, envp, stdin, stdout);
}

static long sys_chdir(const char *path)
{
    assert_nonzero(path);
    assert_userspace(path);
    return vfs_chdir(path);
}

static long sys_getcwd(char *path, int size)
{
    assert_nonzero(path);
    assert_userspace(path);
    return vfs_getcwd(path, size);
}

static long sys_getpid()
{
    process_t *pr;
    pr = process_handle();
    return pr->pid;
}

static long sys_mount(const char *device, const char *fsname, const char *name)
{
    assert_nonzero(device);
    assert_nonzero(fsname);
    assert_nonzero(name);
    assert_userspace(device);
    assert_userspace(fsname);
    assert_userspace(name);
    return vfs_mount(device, fsname, name);
}

static long sys_umount(const char *name)
{
    assert_nonzero(name);
    assert_userspace(name);
    return vfs_umount(name);
}

static long sys_mkdir(const char *path, int mode)
{
    assert_nonzero(path);
    assert_userspace(path);
    return vfs_mkdir(path, mode);
}

static long sys_rmdir(const char *path)
{
    assert_nonzero(path);
    assert_userspace(path);
    return vfs_rmdir(path);
}

static long sys_create(const char *path, int mode)
{
    assert_nonzero(path);
    assert_userspace(path);
    return vfs_create(path, mode);
}

static long sys_remove(const char *path)
{
    assert_nonzero(path);
    assert_userspace(path);
    return vfs_remove(path);
}

static long sys_rename(const char *oldpath, const char *newpath)
{
    assert_nonzero(oldpath);
    assert_userspace(oldpath);
    assert_nonzero(newpath);
    assert_userspace(newpath);
    return vfs_rename(oldpath, newpath);
}

static long sys_gettime(timeval_t *tv)
{
    assert_nonzero(tv);
    assert_userspace(tv);
    return gettime(tv);
}

static void sys_sleep(size_t ns)
{
    // TODO: extremely bad implementation
    timer_sleep(ns / 1000000);
}

/**************************************************************************************/

const void *syscall_table[] = {
    sys_exit,    //  0 = exit
    sys_open,    //  1 = open
    sys_close,   //  2 = close
    sys_read,    //  3 = read
    sys_write,   //  4 = write
    sys_seek,    //  5 = seek
    sys_ioctl,   //  6 = ioctl
    sys_fstat,   //  7 = fstat
    sys_stat,    //  8 = stat
    sys_readdir, //  9 = readdir
    sys_brk,     // 10 = brk
    sys_spawnve, // 11 = spawnve
    sys_wait,    // 12 = wait
    sys_chdir,   // 13 = chdir
    sys_default, // 14 = fchdir
    sys_getcwd,  // 15 = getcwd
    sys_getpid,  // 16 = getpid
    sys_mount,   // 17 = mount
    sys_umount,  // 18 = umount
    sys_mkdir,   // 19 = mkdir
    sys_rmdir,   // 20 = rmdir
    sys_create,  // 21 = create
    sys_remove,  // 22 = remove
    sys_rename,  // 23 = rename
    sys_gettime, // 24 = gettime
    sys_default, // 25 = settime
    sys_sleep,   // 26 = sleep
};

const size_t syscall_count = (sizeof(syscall_table)/sizeof(syscall_table[0]));
extern size_t syscall_address;

// STAR.SYSRET (bits 48:63)
// CS = field + 16, SS = field + 8
// field = 0x13, CS = 0x23, SS = 0x1B

// STAR.SYSCALL (bits 32:47)
// CS = field, SS = field + 8
// field = 0x08, CS = 0x08, SS = 0x10

void syscall_init()
{
    write_msr(MSR_STAR, 0x0013000800000000);
    write_msr(MSR_LSTAR, syscall_address);
    write_msr(MSR_SFMASK, 0);
    write_msr(MSR_GS_BASE, 0);
    write_msr(MSR_FS_BASE, 0);
}
