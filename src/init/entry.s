.text
.globl _entry

_entry:
    csrr tp, mhartid
    addi a0, tp, 1
    li a1, 4096
    mul a0, a0, a1
    la a1, kstack
    add sp, a0, a1
    call start  # jump to target and save position to ra
stop:
    wfi
    j stop
