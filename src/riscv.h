// Machine Status Register, mstatus

#define MSTATUS_MPP_MASK (3L << 11) // previous mode.
#define MSTATUS_MPP_M (3L << 11)
#define MSTATUS_MPP_S (1L << 11)
#define MSTATUS_MPP_U (0L << 11)
#define MSTATUS_MIE (1L << 3)    // machine-mode interrupt enable.
#define MSTATUS_MPIE (1L << 7)
#define MSTATUS_SIE (1L << 1)
#define MSTATUS_SPIE (1L << 5)
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

#define MIE_SSIE (1L << 1)
#define MIE_MSIE (1L << 3)
#define MIE_STIE (1L << 5)
#define MIE_MTIE (1L << 7)
#define MIE_SEIE (1L << 9)
#define MIE_MEIE (1L << 11)
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
w_sstatus(uint64 x)
{
  asm volatile("csrw sstatus, %0" : : "r" (x));
}

#define SSTATUS_SIE (1L << 1)
#define SSTATUS_SPIE (1L << 5)
#define SSTATUS_SPP (1L << 8)
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

#define SIP_SEIP (1L << 9)
#define SIP_STIP (1L << 5)
#define SIP_SSIP (1L << 1)
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