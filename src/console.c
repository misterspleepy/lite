#include "type.h"
#include "sys.h"
#include "file.h"
#define CONS_BUF_SIZE 128
#define BACKSPACE 0x100
#define C(x) ((x) - '@')
static struct {
    uint32 r;
    uint32 w;
    uint32 e;
    int buf[CONS_BUF_SIZE];
    spinlock_t lock;
} uart_rd_buf;

int console_read(int user, uint64 buf, uint32 size)
{
    int n = 0;
    int c;
    char cbuf;
    lock_acquire(&uart_rd_buf.lock);
    while (size > 0) {
        while (uart_rd_buf.r == uart_rd_buf.w) {
            if (killed(myproc())) {
                lock_release(&uart_rd_buf.lock);
                return -1;
            }
            sleep(&uart_rd_buf.r, &uart_rd_buf.lock);
        }
        c = uart_rd_buf.buf[uart_rd_buf.r++ % CONS_BUF_SIZE];
        if (c == C('D')) {
            uart_rd_buf.r--;
            break;
        }
        cbuf = c;
        if (eithercopyout(user, buf, (uint64)&cbuf, 1) == -1) {
            break;
        }
        size--;
        n++;
        buf++;
    }
    lock_release(&uart_rd_buf.lock);
    return n;
}

int console_write(int user, uint64 buf, uint32 size)
{
    int i;
    for (i = 0; i < size; i++) {
        char c;
        if (eithercopyin(user, &c, buf + i, 1) == -1) {
            break;
        }
        uart_putc(c);
    }
    return i;
}

void console_putc(int c)
{
    if (c == BACKSPACE) {
        uart_putc_sync('\b');
        uart_putc_sync(' ');
        uart_putc_sync('\b');
    } else {
        uart_putc_sync(c);
    }
}

void console_intr(int c)
{
    lock_acquire(&uart_rd_buf.lock);
    switch (c) {
        case C('U'): // kill line.
            while (uart_rd_buf.e != uart_rd_buf.w &&
                uart_rd_buf.buf[(uart_rd_buf.e - 1) % CONS_BUF_SIZE] != '\n') {
                uart_rd_buf.e--;
                console_putc(BACKSPACE);
            }
            break;
        case C('H'):
        case '\x7f':
            if (uart_rd_buf.e != uart_rd_buf.w) {
                uart_rd_buf.e--;
                console_putc(BACKSPACE);
            }
            break;
        default:
            if (uart_rd_buf.e - uart_rd_buf.r < CONS_BUF_SIZE) {
                c = (c == '\r') ? '\n' : c;
                console_putc(c);

                uart_rd_buf.buf[uart_rd_buf.e++ % CONS_BUF_SIZE] = c;
                if (c == '\n' || c == C('D') ||
                    uart_rd_buf.e - uart_rd_buf.r == CONS_BUF_SIZE) {
                    uart_rd_buf.w = uart_rd_buf.e;
                    wakeup(&uart_rd_buf.r);
                }
            }
            break;
    };
    lock_release(&uart_rd_buf.lock);
}

void console_init()
{
    uart_rd_buf.r = 0;
    uart_rd_buf.w = 0;
    lock_init(&uart_rd_buf.lock, "uart_rd_buf");
    uart_init();

    devrw[CONSOLE].read = console_read;
    devrw[CONSOLE].write = console_write;
}