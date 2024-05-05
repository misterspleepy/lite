#include "type.h"
#include "conf.h"
#include "memlayout.h"
#include "riscv.h"
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
    char* name;
} spinlock;

typedef uint64 pte;
typedef pte* pagetable_t;

// spinlock
void lock_init(spinlock* lk, char* name);
void lock_acquire(spinlock* lk);
void lock_release(spinlock* lk);
uint64 lock_holding(spinlock* lk);

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
    SLEEP
};

typedef uint32 pid_t;
typedef struct {
    // protected by procs_table.lk
    pid_t pid;
    enum procstate_t state;

    pagetable_t pagetable;
    context_t context;
    void* kstack;
    char name[NBUF];
    trapframe_t* trapframe;
} proc;

typedef struct cpu_t {
    context_t context;
    proc* p;

    // this is process privite data, why not move it to proc
    uint64 noff;
    uint64 sie;
} cpu_t;

void cpu_init();
void proc_init();
void scheduler();
cpu_t* mycpu();
proc* myproc();

// string utils
char* strcpy(char *dest, const char *src);
void* memset(void *str, int c, size_t n);
void* memmove(void *dst, const void *src, size_t n);
// 串口
void uart_putc_sync(char c);
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

// trap
void kernelvec();
void usertrapret();

// syscall
void syscall();
