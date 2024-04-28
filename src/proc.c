#include "sys.h"

proc procs[NPROC];
static pid_t next_pid = 1;
cpu_t cpus[NCPU];

char stacks[3][PAGE_SIZE];
void task(int span)
{
    en_intr();
    const char* p = myproc()->name;
    int c = 0;
    while(1)
    {
        uart_putc_sync('\n');
        const char* p0 = p;
        while (*p0){
            uart_putc_sync(*p0++);
        }
        uart_putc_sync('\t');
        print_uint64(c++);
        int i = span;
        while(i > 0) {
            i--;
        }
    }
}

void task1()
{
    task(50000000);
}
void task2()
{
    task(100000000);
}
void task3()
{
    task(200000000);
}

void proc_init()
{
    for (int i = 0; i < NPROC; i++) {
        procs[i].pid = 0;
        procs[i].state = UNUSED;
        procs[i].stack = 0;
        memset((void*)(&procs[i].context), 0, sizeof(context_t));
        memset(procs[i].name, 0, NBUF);
    }
    const char* names[3] = {
        "proc1",
        "proc2",
        "proc3"
    };
    uint64 tasks[3] = {
        (uint64)task1,
        (uint64)task2,
        (uint64)task3
    };
    for (int i = 0; i < 3; i++) {
        procs[i].pid = next_pid++;
        procs[i].state = RUNNING;
        procs[i].stack = stacks[i];
        strcpy(procs[i].name, names[i]);
        procs[i].context.ra = tasks[i];
        procs[i].context.sp = procs[i].stack + PAGE_SIZE;
    }
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

void yield()
{
    swich(&myproc()->context, &mycpu()->context);
}

void scheduler()
{
    while (1) {
        for (int i = 0; i < NPROC; i++) {
            volatile proc* p =  &procs[i];
            if (p->state != RUNNING) {
                continue;
            }
            cpu_t* cpu = mycpu();
            mycpu()->p = p;
            swich(&mycpu()->context, &p->context);
            dis_intr();
            mycpu()->p = 0;
        }
    }
}