#include "sys.h"
// Fetch content at addr from the current process
// Return 0 for success, or -1 for error
int fetchaddr(uint64 addr, uint64* ip)
{
    proc_t* p = myproc();
    if (addr > p->sz || addr + sizeof(uint64) > p->sz) {
        return -1;
    }
    return copyin(p->pagetable, (uint64)ip, addr, sizeof(uint64));
}

// Fetch the nul-terminated string at addr from the current process.
// Returns length of string, not including nul, or -1 for error.
int fetchstr(uint64 addr, char* buf, uint64 max)
{
    proc_t* p = myproc();
    if (copyinstr(p->pagetable, buf, addr, max) < 0) {
        return -1;
    }
    return strlen(buf);
}

static uint64 argraw(int n)
{
    proc_t* p = myproc();
    switch (n) {
    case 0: return p->trapframe->a0;
    case 1: return p->trapframe->a1;
    case 2: return p->trapframe->a2;
    case 3: return p->trapframe->a3;
    case 4: return p->trapframe->a4;
    case 5: return p->trapframe->a5;
    case 6: return p->trapframe->a6;
    case 7: return p->trapframe->a7;
    }
    panic("argraw");
    return 0;
}

void argint(int n, int* i)
{
    *i = argraw(n);
}
void argaddr(int n, uint64* ip)
{
    *ip = argraw(n);
}
// Fetch the nth word-sized system call argument as a null-terminated string.
// Copies into buf, at most max.
// Returns string length if OK (including nul), -1 if error.
int argstr(int n, char* buf, int max)
{
    uint64 addr;
    argaddr(n, &addr);
    return fetchstr(addr, buf, max);
}
int argfd(int n, int* pfd, struct file** pf);

uint64 sys_sleep()
{
    int n;
    uint32 ticks0;
    argint(0, &n);
    lock_acquire(&time.lk);
    ticks0 = time.ticks;
    while (time.ticks - ticks0 < n) {
        if (killed(myproc())) {
            lock_release(&time.lk);
            return -1;
        }
        sleep(&time, &time.lk);
    }
    lock_release(&time.lk);
    return 0;
}

uint64 sys_fork()
{
    return -1;
}

inode_t* create(const char* path, int type, short major, short minor)
{
    return 0;
}

uint64 sys_mknod()
{
    return -1;
}

// char* path = "init"; char* args[] = {"test1", "test2", 0};
// exec(path, args)
uint64 sys_exec()
{
    char* path[128];
    char* args[ARGSMAX];
    uint64 ret = -1;
    memset(args, 0, sizeof(args));
    uint64 uargs, uarg, argind = 0;
    if (argstr(0, path, PATHMAX) < 0) {
        return -1;
    }
    argaddr(1, &uargs);
    if (uargs == 0) {
        return -1;
    }
    while (argind < ARGSMAX) {
        if (fetchaddr(uargs, &uarg) < 0) {
            goto cleanup;
        }
        if (uarg == 0) {
            break;
        }
        args[argind] = kalloc();
        if (args[argind] == 0) {
            goto cleanup;
        }
        fetchstr(uarg, args[argind], PAGE_SIZE);
        argind++;
        uargs += sizeof(char*);
    }
    if (argind == ARGSMAX) {
        goto cleanup;
    }
    ret = exec(path, args);
cleanup:
    for (int i = 0; i < ARGSMAX; i++) {
        if (args[i] == 0) {
            break;
        }
        kfree(args[i]);
    }
    return ret;
}


uint64 sys_test()
{
    static int c = 0;
    kprintf("sys_test:%d\n", c+=1);
    int i = 100000000;
    while (i-- > 0) {
        ;
    }
    return -1;
}

uint64 (*syscalls[])() = {
    [0] = sys_sleep,
    [1] = sys_test,
    [2] = sys_test,
    [7] = sys_exec
};

void syscall()
{
    int64 syscall_num = myproc()->trapframe->a7;
    if (syscall_num < 0 || sizeof(syscalls) / sizeof(syscalls[0]) <= syscall_num) {
        myproc()->trapframe->a0 = -1;
        return;
    }
    myproc()->trapframe->a0 = syscalls[syscall_num]();
}