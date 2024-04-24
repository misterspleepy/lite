#include "conf.h"
#include "type.h"
#include "riscv.h"
#include "sys.h"
uint8 kstack[NCPU][PAGE_SIZE] __attribute__((aligned (16)));

void start()
{
    uart_init();
    const char* h = "hello world\n";
    const char* p = h;
    while (*p){
        uart_putc_sync(*p++);
    }
    while(1) {
    }
}