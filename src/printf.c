#include <stdarg.h>
#include <sys.h>
static struct {
    int locking;
    spinlock_t lock;
} pr;

int panicked = 0;
static char digits[] = "0123456789abcdef";
static void printint(int xx, int base, int sign)
{
    char buf[16];
    int i;
    uint32 x;
    if (sign && (sign = xx < 0)) {
        x = -xx;
    } else {
        x = xx;
    }

    i = 0;
    do {
        buf[i++] = digits[x % base];
    } while ((x /= base) != 0);

    if (sign) {
        buf[i++] = '-';
    }
    while (--i >= 0) {
        console_putc(buf[i]);
    }
}

static void printptr(uint64 x)
{
  int i;
  console_putc('0');
  console_putc('x');
  for (i = 0; i < (sizeof(uint64) * 2); i++, x <<= 4) {
    console_putc(digits[x >> (sizeof(uint64) * 8 - 4)]);
  }
}


void kprintf(const char* fmt, ...)
{
    va_list ap;
    const char* cp = fmt;
    char c;
    const char* cs;
    if (pr.locking) {
        lock_acquire(&pr.lock);
    }
    va_start(ap, fmt);
    while ((c = *(cp++))  != '\0') {
        if (c != '%') {
            console_putc(c);
            continue;
        }
        if ((c = *(cp++)) == '\0') {
            break;
        }
        switch (c) {
            case 'd':
                printint(va_arg(ap, int), 10, 1);
                break;
            case 'p':
                printptr(va_arg(ap, uint64));
                break;
            case 'x':
                printint(va_arg(ap, int), 16, 1);
                break;
            case 's':
                cs = va_arg(ap, char*);
                if (cs == 0) {
                    cs = "(null)";
                }
                while (*cs != '\0') {
                    console_putc(*cs);
                    cs++;
                }
                break;
            case '%':
                console_putc('%');

            default:
                console_putc('%');
                console_putc(c);
                break;
        }
    }
    va_end(ap);
    if (pr.locking) {
        lock_release(&pr.lock);
    }
}

void panic(const char* s)
{
    pr.locking = 0;
    kprintf("panic: ");
    kprintf(s);
    kprintf("\n");
    panicked = 1;
    for (;;) {
        ;
    }
}

void printf_init()
{
    pr.locking = 1;
    panicked = 0;
    lock_init(&pr.lock, "printf lock");
}