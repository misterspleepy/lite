#include "riscv.h"
#include "memlayout.h"

.section tramsec
# Save trapframe, setup kernel stack, hartid, switch to kernel pagetable, jump to usertrap
.global uservec
.align 4
uservec:
    csrw sscratch, a0
    li a0, TRAP_FRAME

    # save all but a0 from the trapframe
    sd ra, 40(a0)
    sd sp, 48(a0) 
    sd gp, 56(a0)
    sd tp, 64(a0)
    sd t0, 72(a0)
    sd t1, 80(a0)
    sd t2, 88(a0)
    sd s0, 96(a0)
    sd s1, 104(a0)

    sd a1, 112(a0)
    sd a2, 120(a0)
    sd a3, 128(a0)
    sd a4, 136(a0)
    sd a5, 144(a0)
    sd a6, 152(a0)
    sd a7, 160(a0)
    sd s2, 168(a0)
    sd s3, 176(a0)
    sd s4, 184(a0)
    sd s5, 192(a0)
    sd s6, 200(a0)
    sd s7, 208(a0)
    sd s8, 216(a0)
    sd s9, 224(a0)
    sd s10, 232(a0)
    sd s11, 240(a0)
    sd t3, 248(a0)
    sd t4, 256(a0)
    sd t5, 264(a0)
    sd t6, 272(a0)

    # save epc
    csrr t0, sepc
    sd t0, 32(a0)

    # save a0
    mv t0, a0
    csrr a0, sscratch
    sd a0, 280(t0)

    # load hartid
    ld tp, 8(t0)
    
    # load kernel stack
    ld sp, 24(t0)

    # load kernel trapfunc
    ld ra, 16(t0)
    # switch to kernel pagetable
    ld a0, 0(t0)
    sfence.vma zero, zero
    csrw satp, a0
    sfence.vma zero, zero
    # jump to usertrap
    jr ra

# switch to userpagetable, restore trampframe, return to usercode with SIE = ON, MODE = user, pc = epc
.global userret
.align 4
userret:
    sfence.vma zero, zero
    csrw satp, a0
    sfence.vma zero, zero

    li a0, TRAP_FRAME

    # restore all but a0 from the trapframe
    ld ra, 40(a0)
    ld sp, 48(a0) 
    ld gp, 56(a0)
    ld tp, 64(a0)
    ld t0, 72(a0)
    ld t1, 80(a0)
    ld t2, 88(a0)
    ld s0, 96(a0)
    ld s1, 104(a0)

    ld a1, 112(a0)
    ld a2, 120(a0)
    ld a3, 128(a0)
    ld a4, 136(a0)
    ld a5, 144(a0)
    ld a6, 152(a0)
    ld a7, 160(a0)
    ld s2, 168(a0)
    ld s3, 176(a0)
    ld s4, 184(a0)
    ld s5, 192(a0)
    ld s6, 200(a0)
    ld s7, 208(a0)
    ld s8, 216(a0)
    ld s9, 224(a0)
    ld s10, 232(a0)
    ld s11, 240(a0)
    ld t3, 248(a0)
    ld t4, 256(a0)
    ld t5, 264(a0)
    ld t6, 272(a0)

    # restore a0
    ld a0, 280(a0)

    # return to user
    sret
