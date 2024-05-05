#include "sys.h"
void kernelvec();
extern uint32 ticks;
__attribute__((aligned (16))) void main() 
{
    cpu_init();
    kinit();
    w_stvec((uint64)kernelvec);
    uart_init();
    kvminit();
    proc_init();
    scheduler();
}