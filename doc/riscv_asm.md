## Great Idea 1: Abstraction
High level language
Program c

Assembly Language Program

Machine Language Program

## Assembly language
    Basic job of a CPU: execute lots of instructions
    Instructions are the primitive operations that the CPU may execute.
        Like a sentence: operations applied to operands processed in sequence ...
    Different CPUs implement diferent sets of instructions. The set of instructions a particular CPU implements is an Instruction Set Architecture ISA.
        ARM Intel x86

## Book: Programming From the Ground Up

## 

Registers:
    32 general perpose registers
    t0-t7是临时寄存器
    a0-a7 for function arguments
    s0-s11 for saved registers or within function definitions
    sp
    ra
    pc

Stack Pointer Register
    x2 = sp, holds the base address of the stack. sp must aligned to 4 bytes, Failing which, a load/store alignment fault arise

Global Pointer Register
    x3, gp register. will hold the base address of the location where the global variables reside.

Thread Pointer Register
    x4 tp, use it to implememt thread specific variables

Return Address Register
    x1 ra, Before a subroutine call is performed, x1 is explicitly set to the subroutine return address which is usually pc + 4. The standard software calling convention uses x1 ra register to hold the return address on a function call.

Argument Register
8 reigsters, x10-x17, Before a subroutine call is made, the arguments to the subroutine are copied to the argument registers, The stack is used in case the number of arguments exceeds 8.

Temporary Register
    the temporary registers are used to hold intermediate values during instruction execution. t0 - t6

misa
mvendorid
marchid
mimpid
mstatus
mcause
mtvec
mhartid
mepc
mie
mip
mtval
mscratch

CSR Instructions

Register to Register instructions
CSRRC
CSR Read and Clear Bits is used to clear a CSR

csrrc rd, csr, rs1
csrc csr, rs1

Usage: csrrc x1, mcause, zero # mcause <- Invert zero LogicalAnd mcause, x1 <- old value of mcause

CSRR
    CSR Read is used to read from a CSR
    csrr rd, csr
        csrr x5, mstatus
CSRRW
    CSR Read and Write is used to read from and write to a CSR
    csrrw rd, csr, rs1
    alias:csrw csr, rs1
    The preivious value of the CSR is copied to destination register and the value of the source register is copied to the CSR, this is an atomic write operation.

Machine Infomation Registers

物理内存布局

ram start：0x8000 0000
text start：0x8000 0000


text end：

data start
data end

bss start
bss end

mem_alloc_begin
ram_end


entry:
    1. 配置栈寄存器，进入c环境

start：
    1. 中断配置
        代理中断，设置epc，etvec等，为mret做准备
    2. 定时器配置
        机器模式定时器配置，将机器模式的定时器代理到特权模式
    3. 

riscv.h
    c语言操作底层硬件，内嵌asm

memlayout.h


conf.h

sys.h

trapvec.s
    mtimertvec
    kerneltvec

trap.c

uart.c
    串口驱动

kmem.c
    内核空闲页分配器
    kallc
    kinit
    kfree

vm.c
    virtual memory
    
proc.c
    process

vfs.c

syscall.c

main.c
    module init

entry.s

start.c

spinlock.c

file


