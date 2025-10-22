#pragma once

#include <stddef.h>

/*

Syscall instruction

    - https://www.felixcloutier.com/x86/syscall
    - rcx is used for storing return rip
    - r11 is used for storing rflags
    - rflags register is being masked
    - this means that the clobber list is  "cc", "rcx", "r11", "memory"

System V x86-64 ABI

    - rax ("a") is used for syscall id
    - rdi ("D") is arg0
    - rsi ("S") is arg1
    - rdx ("d") is arg2
    - rcx ("c") is arg3
    - r8 is arg4
    - r9 is arg5

Because rcx is used by the syscall instructions we use r10 instead

*/

static inline long syscall(size_t id, size_t arg0, size_t arg1, size_t arg2, size_t arg3, size_t arg4)
{
    long retval;
    register size_t r10 asm("r10") = arg3;
    register size_t r8 asm("r8") = arg4;
    asm volatile("syscall" : "=a"(retval) : "a"(id), "D"(arg0), "S"(arg1), "d"(arg2), "r"(r10), "r"(r8) : "cc", "rcx", "r11", "memory");
    return retval;
}

#define sys_exit(ret) \
    syscall(0, ret, 0, 0, 0, 0)

#define sys_open(path, flags) \
    syscall(1, (size_t)path, flags, 0, 0, 0)

#define sys_close(fd) \
    syscall(2, fd, 0, 0, 0, 0)

#define sys_read(fd, size, buf) \
    syscall(3, fd, size, (size_t)buf, 0, 0)

#define sys_write(fd, size, buf) \
    syscall(4, fd, size, (size_t)buf, 0, 0)

#define sys_seek(fd, offset, origin) \
    syscall(5, fd, offset, origin, 0, 0)

#define sys_ioctl(fd, cmd, val) \
    syscall(6, fd, cmd, (size_t)val, 0, 0)

#define sys_fstat(fd, stat) \
    syscall(7, fd, (size_t)stat, 0, 0, 0)

#define sys_stat(path, stat) \
    syscall(8, (size_t)path, (size_t)stat, 0, 0, 0)

#define sys_readdir(fd, size, dirent) \
    syscall(9, fd, size, (size_t)dirent, 0, 0)

#define sys_brk(brk) \
    syscall(10, brk, 0, 0, 0, 0)

#define sys_spawnve(path, argv, envp, stdin, stdout) \
    syscall(11, (size_t)path, (size_t)argv, (size_t)envp, stdin, stdout)

#define sys_wait(pid, status) \
    syscall(12, pid, (size_t)status, 0, 0, 0)

#define sys_chdir(path) \
    syscall(13, (size_t)path, 0, 0, 0, 0)

#define sys_getcwd(path, size) \
    syscall(15, (size_t)path, size, 0, 0, 0)

#define sys_getpid() \
    syscall(16, 0, 0, 0, 0, 0)

#define sys_mount(source, fstype, target) \
    syscall(17, (size_t)source, (size_t)fstype, (size_t)target, 0, 0)

#define sys_umount(target) \
    syscall(18, (size_t)target, 0, 0, 0, 0)

#define sys_mkdir(path, mode) \
    syscall(19, (size_t)path, mode, 0, 0, 0)

#define sys_rmdir(path) \
    syscall(20, (size_t)path, 0, 0, 0, 0)

#define sys_create(path, mode) \
    syscall(21, (size_t)path, mode, 0, 0, 0)

#define sys_remove(path) \
    syscall(22, (size_t)path, 0, 0, 0, 0)

#define sys_rename(oldpath, newpath) \
    syscall(23, (size_t)oldpath, (size_t)newpath, 0, 0, 0)

#define sys_gettime(tv) \
    syscall(24, (size_t)tv, 0, 0, 0, 0)

#define sys_sleep(ns) \
    syscall(26, ns, 0, 0, 0, 0)

#define sys_sysinfo(req, id, buf, len) \
    syscall(27, req, id, (size_t)buf, len, 0)
