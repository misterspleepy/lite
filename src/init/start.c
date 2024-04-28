#include "conf.h"
#include "type.h"
#include "memlayout.h"
#include "riscv.h"
uint8 kstack[NCPU][PAGE_SIZE] __attribute__((aligned (16)));
uint64 timetemp[4]; // a1, a2, a3, mtimecmp
void mtime_trap_vector();
void main();
void start()
{
    /* init timer interrupt */
    w_mscratch((uint64)timetemp);
    w_mtvec((uint64)(mtime_trap_vector));
    timetemp[3] = 10000000;
    uint64 timecmp = r_mtime();
    timecmp += timetemp[3];
    w_mtimecmp(timecmp);
    w_mstatus(r_mstatus() | MSTATUS_MIE);
    w_mie(r_mie() | MIE_MTIE);

    /* machine mode to supervisor mode */
    uint64 x = r_mstatus();
    x &= ~MSTATUS_MPP_MASK;
    x |= MSTATUS_MPP_S | MSTATUS_MPIE;
    w_mstatus(x); // 设置mstatus：MPP设置为01
    w_mepc((uint64)main); // 设置mepc为main函数
    w_mie(r_mie() | MIE_SEIE | MIE_STIE | MIE_SSIE);
    w_pmpaddr0(0x3fffffffffffffull);
    w_pmpcfg0(0xf);
    w_mideleg(0xffff); // 将中断和异常代理给S mode
    w_medeleg(0xffff); // 将中断和异常代理给S mode
    w_tp(r_mhartid()); // 将hartid存在tp中
    asm volatile("mret");
}