#include "type.h"
#include "conf.h"
#include "memlayout.h"
#include "riscv.h"
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

void swich(context_t* src, context_t* target); // define in swich.s

typedef enum procstate_t {
    UNUSED,
    USED,
    RUNNING,
    SLEEP
};

typedef uint32 pid_t;
typedef struct {
    pid_t pid;
    enum procstate_t state;
    context_t context;
    void* stack;
    char name[NBUF];
    // trapframe
} proc;

typedef struct cpu_t {
    context_t context;
    proc* p;
} cpu_t;

void proc_init();
void scheduler();
cpu_t* mycpu();
proc* myproc();
// string utils
char* strcpy(char *dest, const char *src);
void *memset(void *str, int c, size_t n);
// 串口
void uart_putc_sync(char c);
void print_uint64(uint64 x);
void uart_init();