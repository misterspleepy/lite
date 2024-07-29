#include "sys.h"
#include "elf.h"
static struct {
    proc_t procs[NPROC];
} procs_table;

static struct {
    spinlock_t lk;
    pid_t next_pid;
} npid;

// cpus need not be protected by spinlock
cpu_t cpus[NCPU];

// this func execute in single core.
void cpu_init()
{
    for (int i = 0; i < NCPU; i++) {
        memset(&cpus[i].context, 0, sizeof(context_t));
        cpus[i].noff = 0;
        cpus[i].p = 0;
    }
}

pid_t alloc_pid()
{
    pid_t pid;
    lock_acquire(&npid.lk);
    pid = npid.next_pid++;
    lock_release(&npid.lk);
    return pid;
}

// null for failed.
proc_t* alloc_proc()
{
    for (int i = 0; i < NPROC; i++) {
        lock_acquire(&procs_table.procs[i].lk);
        if (procs_table.procs[i].state == UNUSED) {
            procs_table.procs[i].state = USED;
            procs_table.procs[i].pid = alloc_pid();
            procs_table.procs[i].killed = 0;
            procs_table.procs[i].chan = 0;
            lock_release(&procs_table.procs[i].lk);
            return &procs_table.procs[i];
        }
        lock_release(&procs_table.procs[i].lk);
    }
    return 0;
}

void proc_init()
{
    lock_init(&npid.lk, "pid counter");
    for (int i = 0; i < NPROC; i++) {
        procs_table.procs[i].pid = 0;
        procs_table.procs[i].state = UNUSED;
        procs_table.procs[i].kstack = 0;
        procs_table.procs[i].killed = 0;
        procs_table.procs[i].chan = 0;
        lock_init(&procs_table.procs[i].lk, "proc");
        memset((void*)(&procs_table.procs[i].context), 0, sizeof(context_t));
        memset(procs_table.procs[i].name, 0, NBUF);
    }
    userinit();
}

uint64 cpuid()
{
    return r_tp();
}

cpu_t* mycpu()
{
    uint64 hid = r_tp();
    return &cpus[hid];
}

proc_t* myproc()
{
    return mycpu()->p;
}

// should hold myproc()->lock
// proc's status should change in outside
void sched()
{
    proc_t* proc = myproc();
    if (!lock_holding(&proc->lk)) {
        panic("sched should hold proc lock\n");
    }
    uint64 noff = mycpu()->noff;
    uint64 sie = mycpu()->sie;
    swtch(&proc->context, &mycpu()->context);
    mycpu()->noff = noff;
    mycpu()->sie = sie;
}


// yield must be called in interrupt disable
void yield()
{
    proc_t* proc = myproc();
    if (proc == 0) {
        panic("yield");
    }
    lock_acquire(&proc->lk);
    if (proc->state != RUNNING) {
        panic("yield\n");
    }
    proc->state = RUNNABLE;
    sched();
    lock_release(&proc->lk);
}

void sleep(void* chan, spinlock_t* lk)
{
    proc_t* proc = myproc();
    if (!lock_holding(lk)) {
        panic("sleep");
    }
    lock_acquire(&proc->lk);
    lock_release(lk);
    proc->chan = chan;
    proc->state = SLEEPING;
    sched();
    lock_release(&proc->lk);
    lock_acquire(lk);
}

void wakeup(void *chan)
{
    for (int i = 0; i < NPROC; i++) {
        if (&procs_table.procs[i] == myproc()) {
            continue;
        }
        lock_acquire(&procs_table.procs[i].lk);
        if (procs_table.procs[i].state == SLEEPING && procs_table.procs[i].chan == chan) {
            procs_table.procs[i].state = RUNNABLE;
        }
        lock_release(&procs_table.procs[i].lk);
    }
}

void scheduler()
{
    cpu_t* cpu = mycpu();
    while (1) {
        en_intr();
        dis_intr();
        for (int i = 0; i < NPROC; i++) {
            volatile proc_t* p =  &procs_table.procs[i];
            lock_acquire(&p->lk);
            if (p->state != RUNNABLE) {
                lock_release(&p->lk);
                continue;
            }
            cpu->p = p;
            p->state = RUNNING;
            swtch(&cpu->context, &p->context);
            cpu->p = 0;
            lock_release(&p->lk);
        }
    }
}

uint8 initcode[] = {
  0x17, 0x05, 0x00, 0x00, 0x13, 0x05, 0x45, 0x02,
  0x97, 0x05, 0x00, 0x00, 0x93, 0x85, 0x35, 0x02,
  0x93, 0x08, 0x70, 0x00, 0x73, 0x00, 0x00, 0x00,
  0x93, 0x08, 0x20, 0x00, 0x73, 0x00, 0x00, 0x00,
  0xef, 0xf0, 0x9f, 0xff, 0x2f, 0x69, 0x6e, 0x69,
  0x74, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00
};

void userinit()
{
    proc_t* up = alloc_proc();
    // setup kstack
    up->kstack = kalloc();
    // setup trapframe
    up->trapframe = kalloc();
    // name
    strcpy(up->name, "init");
    // alloc & setup page table
    up->pagetable = kalloc();
    uvmfirst(up->pagetable, initcode, sizeof(initcode));
    up->sz = PAGE_SIZE;
    // arrange for user program run
    up->context.ra = (uint64)forkret;
    up->context.sp = (uint64)up->kstack + PAGE_SIZE;
    up->trapframe->epc = 0;
    mappages(up->pagetable, TRAP_FRAME, up->trapframe, PAGE_SIZE, PTE_R | PTE_W);

    lock_acquire(&up->lk);
    up->state = RUNNABLE;
    lock_release(&up->lk);
}

void uvmfirst(pagetable_t pgtable, const uint8* initcode, size_t size)
{
    mappages(pgtable, TRAMPOLINE, __TRAMPOLINE, PAGE_SIZE, PTE_R | PTE_X);
    uint64 sz;
    sz = uvmalloc(pgtable, 0, PAGE_SIZE, PTE_X | PTE_R | PTE_W);
    if (sz != PAGE_SIZE) {
        panic("uvmfirst\n");
    }
    copyout(pgtable, 0, initcode, size);
}

char killed(proc_t* p)
{
    return p->killed;
}

void setkilled(proc_t* p)
{
    p->killed = 1;
}

void forkret()
{
    static int firt = 1;
    lock_release(&myproc()->lk);
    if (firt) {
        // File system initialization must be run in the context of a
        // regular process (e.g., because it calls sleep), and thus cannot
        // be run from main().
        firt = 0;
        fsinit(0);
    }
    usertrapret();
}

int flags2perm(int flags)
{
    int perm = 0;
    if(flags & ELF_PROG_FLAG_EXEC)
      perm = PTE_X;
    if(flags & ELF_PROG_FLAG_WRITE)
      perm |= PTE_W;
    if(flags & ELF_PROG_FLAG_READ)
      perm |= PTE_R;
    return perm;
}

int exec(const char* path, char* args[])
{
    proc_t* p = myproc();
    struct elfhdr eh;
    struct proghdr phd;
    pagetable_t newptb, oldtb;
    char* ustack[ARGSMAX];
    uint64 sz = 0, filesz = 0, pa, fileoff, paddr, sp, spbase, oldsz, argc;
    uint64 tsz;
    const char *last, *s;
    inode_t* ip = namei(path);
    if (ip == 0) {
        kprintf("exec %s failed, file not found.", path);
        return -1;
    }
    ilock(ip);
    iread(ip, 0, (char*)&eh, sizeof(eh), 0);
    if (eh.magic != ELF_MAGIC) {
        kprintf("exec %s failed, invalid format.", path);
        goto exec_bad;
    }
    newptb = kalloc();
    if (newptb == 0) {
        kprintf("exec %s failed, alloc memory failed.", path);
        goto exec_bad;
    }
    mappages(newptb,TRAMPOLINE, __TRAMPOLINE, PAGE_SIZE, PTE_R | PTE_X);
    mappages(newptb, TRAP_FRAME, p->trapframe, PAGE_SIZE, PTE_R | PTE_W);
    // load file
    for (int i = 0; i < eh.phnum; i++) {
        if (iread(ip, 0, (char*)&phd, sizeof(phd), eh.phoff + i * eh.phentsize) != sizeof(phd)) {
            kprintf("exec %s failed, read program header entry failed", path);
            goto exec_bad;
        }
        if (phd.type != ELF_PROG_LOAD) {
            continue;
        }
        if ((sz = uvmalloc(newptb, sz, phd.paddr + phd.memsz, flags2perm(phd.flags))) != phd.paddr + phd.memsz) {
            kprintf("exec %s failed, read program header entry failed", path);
            goto exec_bad;
        }
        fileoff = phd.off;
        paddr = phd.paddr;
        pa = walkaddr(newptb, paddr);
        pa += phd.paddr & (PAGE_SIZE - 1);
        filesz = phd.filesz;
        tsz = MIN(PAGE_SIZE - (phd.paddr & (PAGE_SIZE - 1)), filesz);
        while (1) {
            if (iread(ip, 0, pa, tsz, fileoff) != tsz) {
                kprintf("exec %s failed, can't load program segment.", path);
                goto exec_bad;
            }
            filesz -= tsz;
            if (filesz == 0) {
                break;
            }
            fileoff += tsz;
            paddr += tsz;
            if (paddr & (PAGE_SIZE - 1)) {
                panic("exec");
            }
            pa = walkaddr(newptb, paddr);
            tsz = MIN(PAGE_SIZE, filesz);
        }
    }
    // setup stack
    if (uvmalloc(newptb, sz, 
        PAGE_ROUNDUP(sz) + 2 * PAGE_SIZE, PTE_R | PTE_W) != PAGE_ROUNDUP(sz) + 2 * PAGE_SIZE) {
        goto exec_bad;
    }
    sz = PAGE_ROUNDUP(sz) + 2 * PAGE_SIZE;
    iunlock(ip);
    iput(ip);
    uvmclear(newptb, sz - 2 * PAGE_SIZE); // set up stack gard, prevent from stack overflow
    sp = sz;
    spbase = sp - PAGE_SIZE;
    argc = 0;
    for (argc = 0; args[argc] != 0; argc++) {
        if (argc >= ARGSMAX) {
            break;
        }
        sp -= strlen(args[argc]) + 1;
        sp -= sp % 16; // stack shold be 16 byte alignment
        if (sp < spbase) {
            kprintf("alloc args failed, sp:%p, spbase:%p\n", sp, spbase);
            goto exec_bad;
        }
        if (copyout(newptb, sp, args[argc],
            strlen(args[argc]) + 1) != 0) {
            kprintf("copyout failed\n");
            goto exec_bad;
        }
        ustack[argc] = (char*)sp;
    }
    sp -= sizeof(char*) * (argc + 1);
    sp -= sp % 16;
    if (sp < spbase) {
        kprintf("exec failed, failed to alloc args");
        goto exec_bad;
    }
    if (copyout(newptb, sp, ustack,
        sizeof(char*) * (argc + 1)) != 0) {
        kprintf("exec failed, failed to copy out utack");
        goto exec_bad;
    }
    p->trapframe->a1 = sp;
    // Save program name for debugging.
    for(last=s=path; *s; s++) {
        if(*s == '/') {
            last = s+1;
        }
    }
    strcpy(p->name, last);
    // Commit to the user image.
    oldtb = p->pagetable;
    oldsz = p->sz;
    p->pagetable = newptb;
    p->sz = sz;
    p->trapframe->epc = eh.entry;  // initial program counter = main
    p->trapframe->sp = sp; // initial stack pointer
    uvmfree(oldtb, oldsz);
    p->trapframe->epc = eh.entry;
    return argc;
exec_bad:
    if (newptb) {
        uvmfree(newptb, sz);
    }
    if (sleeplock_holding(&ip->lock)) {
        iunlock(ip);
        iput(ip);
    }
    while (*args) {
        kfree(*args);
        args++;
    }
    return -1;
}