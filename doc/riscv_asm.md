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
        代理中断，trap到S模式
            medeleg：
                1 ： MSIP
                5 ： MTIP
                9 ： MEIP
            mideleg：
                0 ： 0-9 12-13 15
        machine mode:
            mtvec: 中断向量
                汇编代码
                保存现场
                触发S模式的软件中断
                设置mtimecompaire
                恢复现场
                返回到trap点
        timer interrupt:
            scratch register: 在c程序中需要一个register，保存mtime mtimecompare
            IE TI
        supervisor mode：
            start不配置

    2. 特权模式转换成S：
        mepc：保存main函数的地址
        模拟函数调用：main的参数， main函数的返回地址设置：a1,a2... ra
        mpp: 设置为S模式：0：User mode，1：Supervisor mode，3，Machine mode
        通过mret返回到S
    
    3. 配置PMP

kmem.c
    用于物理内存分配
    kinit
    kalloc
    kfree

proc.c
    enum state {
        invalid,
        running,
        sleep,

    }
    struc proc {
        pid_t pid,
        void* chan,
        state state,
        pagetable* pgtb,
        void* kstack,
        struct context* con,
        trapframe* 
        struct file ofiles[NFILE]
    }

main.c
    线程的概念：初始化只有一个线程，初始化完成后kernel中只有scheduler线程，其他都是进程的kernel态线程
    trap中的线程：啥概念
    struct cpu {
        struct Context context
    }
    Cpu cpus[NCPU]
    

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


