#include "sys.h"

static struct {
    spinlock lk;
    proc procs[NPROC];
} procs_table;

static struct {
    spinlock lk;
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
proc* alloc_proc()
{
    lock_acquire(&procs_table.lk);
    for (int i = 0; i < NPROC; i++) {
        if (procs_table.procs[i].state == UNUSED) {
            procs_table.procs[i].state = USED;
            procs_table.procs[i].pid = alloc_pid();
            lock_release(&procs_table.lk);
            return &procs_table.procs[i];
        }
    }
    lock_release(&procs_table.lk);
    return 0;
}

void proc_init()
{
    lock_init(&npid.lk, "pid counter");
    lock_init(&procs_table.lk, "procs_table");
    for (int i = 0; i < NPROC; i++) {
        procs_table.procs[i].pid = 0;
        procs_table.procs[i].state = UNUSED;
        procs_table.procs[i].kstack = 0;
        memset((void*)(&procs_table.procs[i].context), 0, sizeof(context_t));
        memset(procs_table.procs[i].name, 0, NBUF);
    }
    userinit();
}

cpu_t* mycpu()
{
    uint64 hid = r_tp();
    return &cpus[hid];
}

proc* myproc()
{
    return mycpu()->p;
}

// yield must be called in interrupt disable
void yield()
{
    uint64 noff = mycpu()->noff;
    uint64 sie = mycpu()->sie;
    swtch(&myproc()->context, &mycpu()->context);
    mycpu()->noff = noff;
    mycpu()->sie = sie;
}

void scheduler()
{
    cpu_t* cpu = mycpu();
    while (1) {
        en_intr();
        dis_intr();
        lock_acquire(&procs_table.lk);
        for (int i = 0; i < NPROC; i++) {
            volatile proc* p =  &procs_table.procs[i];
            if (p->state != RUNNABLE) {
                continue;
            }
            cpu->p = p;
            p->state = RUNNING;
            lock_release(&procs_table.lk);
            swtch(&cpu->context, &p->context);
            lock_acquire(&procs_table.lk);
            p->state = RUNNABLE;
            lock_release(&procs_table.lk);
            cpu->p = 0;
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
    proc* up = alloc_proc();
    // setup kstack
    up->kstack = kalloc();
    // setup trapframe
    up->trapframe = kalloc();
    // name
    strcpy(up->name, "init");
    // alloc & setup page table
    up->pagetable = kalloc();
    uvmfirst(up->pagetable, initcode, sizeof(initcode));
    // arrange for user program run
    up->context.ra = (uint64)usertrapret;
    up->context.sp = (uint64)up->kstack + PAGE_SIZE;
    up->trapframe->epc = 0;
    mappages(up->pagetable, TRAP_FRAME, up->trapframe, PAGE_SIZE, PTE_R | PTE_W);

    lock_acquire(&procs_table.lk);
    up->state = RUNNABLE;
    lock_release(&procs_table.lk);
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