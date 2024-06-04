#include "sys.h"
#define TIME_INTR 1
#define EXTERN_INTR 2

uint32 ticks = 0;
void timerintr()
{
    ticks++;
}
extern uint64 timestamp;
void yield();
uint64 devinterrupt(uint64 cause)
{
    if (cause == 0x8000000000000001) {
        timerintr();
        w_sip(r_sip() & (~SIP_SSIP)); // 取消pending
        return TIME_INTR;
    }
    if (cause == 0x8000000000000009) {
        // supervisor external interrupt
        // PLIC things
        return EXTERN_INTR;
    }
    return 0;
}
void ktrap()
{
    volatile uint64 cause = r_scause();
    int whichdev = devinterrupt(cause);
    if (whichdev == 0) {
        // unknown exceptions, should print the message.
        panic("unknown exceptions");
    }
    if (whichdev == TIME_INTR) {
        yield();
    }
}

// uservec execute "j usertrap"
// user trap environment is saved, pagetable is kervel pagetable, interrupt is disabled
// current stack pointer set to myproc()->kstack
// hartid is save in tp, myproc() is valid.
void usertrap()
{
    // switch trap handler to kernelvec
    w_stvec((uint64)kernelvec);

    // setup noff, this is critial to spinlock
    mycpu()->noff = 0;
    // handle device interrupts or exceptions
    volatile uint64 cause = r_scause();
    uint64 whichdev = 0;
    if (cause == 8) {
        // syscall handler
        // check whether process is killed, if so, exit current process
        myproc()->trapframe->epc += 4; // exception don't move to next instruction
        en_intr();
        syscall();
        dis_intr();
    } else if(whichdev = devinterrupt(cause) == 0) {
        // unknown exceptions, should print the message.
        // kill current process
        panic("unknown exceptions");
    } else if (whichdev == TIME_INTR) {
        yield();
    }
    // check whether process is killed, if so, exit current process
    // prepair for userret
    usertrapret();
}

void uservec();
void userret(uint64 satp);
// disable interrupts
// set trap handler to uservec
// arrange kernel_satp, hartid, trapfunc, stack for next user interrupt or exception
// set sepc to trapframe->epc, the entry point for usercode
// setup SPIE and SPP, and supply userpagetable to userret.
void usertrapret()
{
    uint64 satp;
    proc_t* p = myproc();
    // 停用中断，等待到usermode的时候才开启
    dis_intr();
    // 设置中断handler为uservec
    void (*uservec_ptr)();
    uservec_ptr = ((uint64)uservec - (uint64)__TRAMPOLINE) + (uint64)TRAMPOLINE; 
    w_stvec((uint64)uservec_ptr);
    // 配置kernel_satp, hartid, trapfunc, stack, sp
    p->trapframe->kernel_hartid = r_tp();
    p->trapframe->kernel_satp = r_satp();
    p->trapframe->kernel_trapfunc = (uint64)usertrap;
    p->trapframe->kernel_sp = (uint64)p->kstack + PAGE_SIZE;
    // 配置epc,使其指向process用户空间的代码行
    w_sepc(p->trapframe->epc);
    // 配置特权级以及中断
    w_sstatus((r_sstatus() | SSTATUS_SPIE) & (~SSTATUS_SPP));
    // 制作satp，调用userret进入汇编代码
    satp = MAKE_SAPT(p->pagetable);
    void (*userret_ptr)(uint64 satp);
    userret_ptr = (uint64)TRAMPOLINE; 
    userret_ptr = ((uint64)userret - (uint64)__TRAMPOLINE) + (uint64)TRAMPOLINE;
    userret_ptr(satp);
}