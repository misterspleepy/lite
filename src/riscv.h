// Machine Status Register, mstatus
#define MSTATUS_MPP_MASK (3L << 11) // previous mode.
#define MSTATUS_MPP_M (3L << 11)
#define MSTATUS_MPP_S (1L << 11)
#define MSTATUS_MPP_U (0L << 11)
#define MSTATUS_MIE (1L << 3)    // machine-mode interrupt enable.
#define MSTATUS_MPIE (1L << 7)
#define MSTATUS_SIE (1L << 1)
#define MSTATUS_SPIE (1L << 5)

#define MIE_SSIE (1L << 1)
#define MIE_MSIE (1L << 3)
#define MIE_STIE (1L << 5)
#define MIE_MTIE (1L << 7)
#define MIE_SEIE (1L << 9)
#define MIE_MEIE (1L << 11)

#define SSTATUS_SIE (1L << 1)
#define SSTATUS_SPIE (1L << 5)
#define SSTATUS_SPP (1L << 8)

#define SIP_SEIP (1L << 9)
#define SIP_STIP (1L << 5)
#define SIP_SSIP (1L << 1)

// one beyond the highest possible virtual address.
// MAXVA is actually one bit less than the max allowed by
// Sv39, to avoid having to sign-extend virtual addresses
// that have the high bit set.
#define MAXVA (1L << (9 + 9 + 9 + 12 - 1))
#define PAGE_SIZE 0x1000
#define PAGE_SHIFT 12
#define PAGE_ROUNDDOWN(a) ((a) & (~(PAGE_SIZE - 1)))
#define PAGE_ROUNDUP(a) (((a) + (PAGE_SIZE - 1)) & (~(PAGE_SIZE - 1)))
#define PTE_V (1L << 0)
#define PTE_R (1L << 1)
#define PTE_W (1L << 2)
#define PTE_X (1L << 3)
#define PTE_U (1L << 4)
#define PTE_G (1L << 5)
#define PTE_A (1L << 6)
#define PTE_D (1L << 7)
#define PTE2PA(pte) ((pte >> 10) << 12)
#define PA2PTE(pa) ((pa >> 12) << 10)
#define VPN(va, i) ((va >> (9 * i + 12)) & 0x1ff)

#ifndef __ASSEMBLER__
// only c code can include these funcs
static inline uint64
r_mstatus()
{
  uint64 x;
  asm volatile("csrr %0, mstatus" : "=r" (x) );
  return x;
}

static inline void 
w_mstatus(uint64 x)
{
  asm volatile("csrw mstatus, %0" : : "r" (x));
}

static inline uint64 
r_mie()
{
  uint64 x;
  asm volatile("csrr %0, mie" : "=r" (x) );
  return x;
}

static inline void 
w_mie(uint64 x)
{
  asm volatile("csrw mie, %0" : : "r" (x));
}

static inline void
w_mtvec(uint64 x)
{
  asm volatile("csrw mtvec, %0" : : "r" (x));
}

// Physical Memory Protection
static inline void
w_pmpcfg0(uint64 x)
{
  asm volatile("csrw pmpcfg0, %0" : : "r" (x));
}

static inline void
w_pmpaddr0(uint64 x)
{
  asm volatile("csrw pmpaddr0, %0" : : "r" (x));
}

static inline void
w_mepc(uint64 x)
{
  asm volatile("csrw mepc, %0" : : "r" (x));
}

static inline uint64
r_mepc()
{
  uint64 x;
  asm volatile("csrr %0, mepc" : "=r" (x));
  return x;
}

static inline void
w_mideleg(uint64 x)
{
  asm volatile("csrw mideleg, %0" : : "r" (x));
}

static inline void
w_medeleg(uint64 x)
{
  asm volatile("csrw medeleg, %0" : : "r" (x));
}

static inline void
w_mip(uint64 x)
{
  asm volatile("csrw mip, %0" : : "r" (x));
}

static inline uint64
r_mip()
{
  uint64 x;
  asm volatile("csrr %0, mip" : "=r" (x));
  return x;
}

static inline void
w_mscratch(uint64 x)
{
  asm volatile("csrw mscratch, %0" : : "r" (x));
}

static inline uint64
r_mhartid()
{
  uint64 x;
  asm volatile("csrr %0, mhartid" : "=r" (x));
  return x;
}

static inline void
w_mtimecmp(uint64 x)
{
  uint64 hartid = r_mhartid();
  *(uint64*)(MTIME_CMP_BASE + hartid * 4) = x;
}

static inline uint64
r_mtimecmp(uint64 x)
{
  uint64 hartid = r_mhartid();
  return *(uint64*)(MTIME_CMP_BASE + hartid * 4);
}

static inline uint64
r_mtime()
{
  return *(uint64*)(MTIME);
}

static inline void
w_stvec(uint64 x)
{
  asm volatile("csrw stvec, %0" : : "r" (x));
}

static inline void
w_sepc(uint64 x)
{
  asm volatile("csrw sepc, %0" : : "r" (x)); 
}

static inline void
w_sstatus(uint64 x)
{
  asm volatile("csrw sstatus, %0" : : "r" (x));
}

static inline uint64
r_sstatus()
{
  uint64 x;
  asm volatile("csrr %0, sstatus" : "=r"(x));
  return x;
}

static inline uint64
r_scause()
{
  uint64 x;
  asm volatile("csrr %0, scause" : "=r"(x));
  return x;
}

static inline uint64
r_sip()
{
  uint64 x;
  asm volatile("csrr %0, sip" : "=r"(x));
  return x;
}

static inline void
w_sip(uint64 x)
{
  asm volatile("csrw sip, %0" : : "r" (x));
}

static inline uint64
r_tp()
{
  uint64 x;
  asm volatile("add %0, tp, zero" : "=r"(x));
  return x;
}

static inline void
w_tp(uint64 x)
{
  asm volatile("add tp, %0, zero" : : "r"(x));
}
static inline void
en_intr()
{
  w_sstatus(r_sstatus() | SSTATUS_SIE);
}
static inline void
dis_intr()
{
  w_sstatus(r_sstatus() & (~SSTATUS_SIE));
}

// flush the TLB.
static inline void
sfence_vma()
{
  // the zero, zero means flush all TLB entries.
  asm volatile("sfence.vma zero, zero");
}

#define SATP_SV39 (8L << 60)
#define MAKE_SAPT(pa) (SATP_SV39 | (((uint64)pa) >> 12))

static inline void
w_satp(uint64 satp)
{
  asm volatile("csrw satp, %0" : :"r"(satp));
}

static inline uint64
r_satp()
{
  uint64 x;
  asm volatile("csrr %0, satp" : "=r"(x));
  return x;
}

#endif