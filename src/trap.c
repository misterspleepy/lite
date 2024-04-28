#include "type.h"
#include "memlayout.h"
#include "riscv.h"
uint32 ticks = 0;
void timerintr()
{
    ticks++;
}
extern uint64 timestamp;
void yield();
uint64 devinterrupt()
{
    volatile uint64 cause = r_scause();
    if (cause == 0x8000000000000001) {
        timerintr();
        w_sip(r_sip() & (~SIP_SSIP)); // 取消pending
        return 1;
    }
    return 0;
}
void ktrap()
{
    int whichdev = devinterrupt();
    if (whichdev == 1) {
        yield();
    }
}
