#include "memlayout.h"
#include "sys.h"
#define Reg(reg) ((volatile unsigned char *)(UART0_BASE + reg))
#define ReadReg(reg) (*(Reg(reg)))
#define WriteReg(reg, v) (*(Reg(reg)) = (v))
#define RHR 0                 // receive holding register (for input bytes)
#define THR 0                 // transmit holding register (for output bytes)
#define IER 1                 // interrupt enable register
#define IER_RX_ENABLE (1<<0)
#define IER_TX_ENABLE (1<<1)
#define FCR 2                 // FIFO control register
#define FCR_FIFO_ENABLE (1<<0)
#define FCR_FIFO_CLEAR (3<<1) // clear the content of the two FIFOs
#define ISR 2                 // interrupt status register
#define LCR 3                 // line control register
#define LCR_EIGHT_BITS (3<<0)
#define LCR_BAUD_LATCH (1<<7) // special mode to set baud rate
#define LSR 5                 // line status register
#define LSR_RX_READY (1<<0)   // input is waiting to be read from RHR
#define LSR_TX_IDLE (1<<5)    // THR can accept another character to send

#define LCR_THRE 5

#define TX_BUF_SIZE 8
static struct {
  uint32 r;
  uint32 w;
  char buf[TX_BUF_SIZE];
  spinlock_t lock;
} uart_tx_buf;

void uart_init()
{
 // disable interrupts.
  WriteReg(IER, 0x00);

  // special mode to set baud rate.
  WriteReg(LCR, LCR_BAUD_LATCH);

  // LSB for baud rate of 38.4K.
  WriteReg(0, 0x03);

  // MSB for baud rate of 38.4K.
  WriteReg(1, 0x00);

  // leave set-baud mode,
  // and set word length to 8 bits, no parity.
  WriteReg(LCR, LCR_EIGHT_BITS);

  // reset and enable FIFOs.
  WriteReg(FCR, FCR_FIFO_ENABLE | FCR_FIFO_CLEAR);

  // enable transmit and receive interrupts.
  WriteReg(IER, IER_TX_ENABLE | IER_RX_ENABLE);

  lock_init(&uart_tx_buf.lock, "uart tx bif log");
  uart_tx_buf.r = 0;
  uart_tx_buf.w = 0;
}

void uart_putc_sync(char c)
{
  push_off();
  while (!(ReadReg(LSR) & LSR_TX_IDLE))
  {}
  WriteReg(THR, c);
  pop_off();
}

// uart_tx_buf.lock need to be holded
void uart_start()
{
  char c;
  while (1) {
    if (uart_tx_buf.r == uart_tx_buf.w) {
      return;
    }
    if ((ReadReg(LSR) & LSR_TX_IDLE) == 0) {
      return;
    }
    c = uart_tx_buf.buf[(uart_tx_buf.r++) % TX_BUF_SIZE];
    wakeup(&uart_tx_buf.r);
    WriteReg(THR, c);
  }
}

void uart_putc(char c)
{
  lock_acquire(&uart_tx_buf.lock);
  while(uart_tx_buf.r + TX_BUF_SIZE ==  uart_tx_buf.w) {
    sleep(&uart_tx_buf.r, &uart_tx_buf.lock);
  }
  uart_tx_buf.buf[(uart_tx_buf.w++) % TX_BUF_SIZE] = c;
  uart_start();
  lock_release(&uart_tx_buf.lock);
}

int uart_getc()
{
  if (ReadReg(LSR) & LSR_RX_READY) {
    return ReadReg(RHR);
  }
  return -1;
}

extern void console_int(int);

void uartintr()
{
  int c;
  while (1) {
    c = uart_getc();
    if (c == -1) {
      break;
    }
    console_intr(c);
  }
  lock_acquire(&uart_tx_buf.lock);
  uart_start();
  lock_release(&uart_tx_buf.lock);
}