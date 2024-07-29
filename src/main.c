#include "sys.h"

void kernelvec();
extern uint32 ticks;
__attribute__((aligned (16))) void main() 
{
    lock_init(&time.lk, "time");
    time.ticks = 0;
    cpu_init();
    kinit();
    w_stvec((uint64)kernelvec);
    kvminit();
    proc_init();
    plic_init();
    plic_inithart();
    console_init();
    binit();
    virtio_disk_init();
    scheduler();
}