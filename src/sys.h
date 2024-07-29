#include "type.h"
#include "conf.h"
#include "memlayout.h"
#include "riscv.h"
#include "fs.h"
extern char __TRAMPOLINE[];
extern char __TEXT_BEGIN[];
extern char __TEXT_END[];
extern char __DATA_BEGIN[];
extern char __DATA_END[];
extern char __KERNEL_END[];
extern uint32 ticks;

typedef struct {
    uint64 locked;
    uint64 cpu;
    const char* name;
} spinlock_t;

typedef uint64 pte_t;
typedef pte_t* pagetable_t;

// spinlock
void lock_init(spinlock_t* lk, const char* name);
void lock_acquire(spinlock_t* lk);
void lock_release(spinlock_t* lk);
uint64 lock_holding(spinlock_t* lk);

void push_off();
void pop_off();

// proc
typedef struct {
    uint64 ra;
    uint64 sp;

    uint64 s0;
    uint64 s1;
    uint64 s2;
    uint64 s3;
    uint64 s4;
    uint64 s5;
    uint64 s6;
    uint64 s7;
    uint64 s8;
    uint64 s9;
    uint64 s10;
    uint64 s11;
} context_t;

typedef struct {
    uint64 kernel_satp;
    uint64 kernel_hartid;
    uint64 kernel_trapfunc;
    uint64 kernel_sp;
    uint64 epc;

    uint64 ra;
    uint64 sp;
    uint64 gp;
    uint64 tp;
    uint64 t0;
    uint64 t1;
    uint64 t2;
    uint64 s0;
    uint64 s1;
    // uint64 a0;
    uint64 a1;
    uint64 a2;
    uint64 a3;
    uint64 a4;
    uint64 a5;
    uint64 a6;
    uint64 a7;
    uint64 s2;
    uint64 s3;
    uint64 s4;
    uint64 s5;
    uint64 s6;
    uint64 s7;
    uint64 s8;
    uint64 s9;
    uint64 s10;
    uint64 s11;
    uint64 t3;
    uint64 t4;
    uint64 t5;
    uint64 t6;

    uint64 a0;
} trapframe_t;

void swtch(context_t* src, context_t* target); // define in swich.s

typedef enum procstate_t {
    UNUSED,
    USED,
    RUNNABLE,
    RUNNING,
    SLEEPING,
    ZOMBIE
};

typedef uint32 pid_t;
typedef struct {
    pid_t pid;
    enum procstate_t state;
    void* chan; // sleeping channel
    spinlock_t lk;

    inode_t* pwd;
    char killed;
    pagetable_t pagetable;
    uint64 sz; // va size
    context_t context;
    void* kstack;
    char name[NBUF];
    trapframe_t* trapframe;
} proc_t;

typedef struct cpu_t {
    context_t context;
    proc_t* p;

    // this is process privite data, why not move it to proc
    uint64 noff;
    uint64 sie;
} cpu_t;

void cpu_init();
void proc_init();
void scheduler();
uint64 cpuid();
cpu_t* mycpu();
proc_t* myproc();
void sleep(void* chan, spinlock_t* lk);
void wakeup(void* chan);
char killed(proc_t* p);
void setkilled(proc_t* p);
void forkret();
typedef struct {
    uint32 ticks;
    spinlock_t lk;
} time_t;
extern time_t time;

// string utils
char* strcpy(char *dest, const char *src);
void* memset(void *str, int c, size_t n);
void* memmove(void *dst, const void *src, size_t n);
int strlen(const char *s);
// 串口
void uart_putc_sync(char c);
void uart_putc(char c);
void print_int(uint64 x);
void print_hex(uint64 x);
void print_str(const char* msg);
void uart_init();

// utils
void panic(const char* msg);
#define MIN(x, y) (x < y ? x : y)
#define MAX(x, y) (x > y ? x : y)
// kmem
void kinit();
void* kalloc();
void kfree(void*);
void kvminit();
extern pagetable_t kernel_page_table;
void mappages(pagetable_t pgtable, uint64 va, uint64 pa, uint64 size, uint8 flag);
void freewalk(pagetable_t pagetable);
void uvmclear(pagetable_t ptb, uint64 va);

// trap
void kernelvec();
void usertrapret();

// syscall
void syscall();

// sleep_lock.h
typedef struct {
    int32 locked;
    spinlock_t spinlock;
    proc_t* myproc;
    const char* name;
} sleeplock_t;
void sleeplock_init(sleeplock_t* lk, const char* name);
void sleeplock_acquire(sleeplock_t* lk);
void sleeplock_release(sleeplock_t* lk);
int32 sleeplock_holding(sleeplock_t* lk);

typedef struct inode {
    uint32 ino;
    uint32 dev;
    uint32 refcount;
    sleeplock_t lock;
    char valid;
    dinode_t dinode;
} inode_t;


// bio
struct buf {
    struct buf* prev;
    struct buf* next;
    int32 ref_count;
    int32 dev;
    int32 block_num;

    sleeplock_t slk;
    int32 is_valid;
    int32 disk;
    char data[BLOCK_SIZE];
};
typedef struct buf buf_t;
void binit();
buf_t* bread(int32 dev, int32 block_num);
void bwrite(buf_t* buf);
void brelease(buf_t* buf);
void bpin(buf_t *b);
void bunpin(buf_t *b);

// virtio_disk
void virtio_disk_rw(buf_t* buf, int32 write);
void virtio_disk_init();
void virtio_disk_intr();

// plic
void plic_init();
void plic_inithart();
int plic_claim(void);
void plic_complete(int irq);

uint64 copyout(pagetable_t pagetable, uint64 dstva, uint64 src, uint64 size);
uint64 copyin(pagetable_t pg, uint64 dst, uint64 srcva, uint64 size);
uint64 copyinstr(pagetable_t pg, uint64 dst, uint64 srcva, uint64 max);
uint64 eithercopyin(short user, uint64 dst, uint64 srcva, uint64 size);
uint64 eithercopyout(short user, uint64 dstva, uint64 src, uint64 size);
void uvmfree(pagetable_t ptb, uint64 size);
uint64 uvmalloc(pagetable_t pgtable, uint64 oldsz, uint64 newsz, uint8 xperm);
uint64 walkaddr(pagetable_t pgtable, uint64 va);
void uvmunmap(pagetable_t pagetable, uint64 va, uint64 npages, int do_free);

void console_init();
void uartintr();