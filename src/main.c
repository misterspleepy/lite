#include "sys.h"
void kernelvec();
extern uint32 ticks;
__attribute__((aligned (16))) void main() 
{
    w_stvec((uint64)kernelvec);
    uart_init();

    proc_init();
    scheduler();
}