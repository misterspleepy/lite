// Physical memory layout

// qemu -machine virt is set up like this,
// based on qemu's hw/riscv/virt.c:
//
// 00001000 -- boot ROM, provided by qemu
// 02000000 -- CLINT
// 0C000000 -- PLIC
// 10000000 -- uart0 
// 10001000 -- virtio disk 
// 80000000 -- boot ROM jumps here in machine mode
//             -kernel loads the kernel here
// unused RAM after 80000000.

// the kernel uses physical memory thus:
// 80000000 -- entry.S, then kernel text and data
// end -- start of kernel page allocation area
// PHYSTOP -- end RAM used by the kernel

// qemu puts UART registers here in physical memory.
#define CLINI_BASE (0x2000000L)
#define CLINI_SIZE (0x10000L)

#define PLIC_BASE 0x0c000000L
#define PLIC_SIZE (0x400000L)

#define UART0_BASE (0x10000000L)
#define UART0_SIZE (0x1000L)

#define VIRTIO_BASE (0x10001000L)
#define VIRTIO_SIZE (0x1000L)

#define MTIME_CMP_BASE 0x2004000L
#define MTIME 0x200bff8L

#define KERNBASE 0x80000000L
#define PHYSTOP (KERNBASE + 128*1024*1024)


#define TRAMPOLINE (MAXVA - PAGE_SIZE)
#define TRAP_FRAME (TRAMPOLINE - PAGE_SIZE)
