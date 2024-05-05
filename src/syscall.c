#include "sys.h"

void syscall()
{
    // fixme:this should do something useful
    print_str("===syscall, pid:");
    print_int(myproc()->pid);
    print_str("; sysnum:");
    print_int(myproc()->trapframe->a7);
    uint32 next_ticks = ticks + 1;
    while (ticks < next_ticks) {
    }
}