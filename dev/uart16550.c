/* Simple 16550 compatible UART routines */

#include "uart.h"
#include "board.h"

struct st_uart16550
{
  unsigned int RTX;
  unsigned int IER;
  unsigned int IIR;
  unsigned int LCR;
  unsigned int MCR;
  unsigned int LSR;
  unsigned int MSR;
  unsigned int SCR;
};

static const struct st_uart16550* devices[] = {
  (struct st_uart16550 *) sys_UART0_BASE,
  (struct st_uart16550 *) sys_UART1_BASE,
  (struct st_uart16550 *) sys_GPS_BASE
};

#define UART16550(n) (*(volatile struct st_uart16550 *) devices[n])

void
uart_tx (int dev, unsigned char data)
{
  while (!((UART16550(dev).LSR) & 0x20));
  UART16550(dev).RTX = data;
}

unsigned char
uart_rx (int dev)
{
  while (!((UART16550(dev).LSR) & 0x1));
  return (UART16550(dev).RTX & 0xff);
}

int uart_rx_no_block (int dev, unsigned char *c) {
  if (((UART16550(dev).LSR) & 0x1)) {
    *c = (UART16550(dev).RTX & 0xff);
    return 0;
  }
  return -1;
}

unsigned char
uart_rx_echo (int dev)
{
  unsigned char data;

  while (!(UART16550(dev).LSR & 1));
  data = UART16550(dev).RTX & 0xff;

  while (!(UART16550(dev).LSR & 0x20));
  UART16550(dev).RTX = data;
  return data;
}

void
uart_rx_flush (int dev)
{
  while (UART16550(dev).LSR & 1) UART16550(dev).RTX;
}

void
uart_set_baudrate (int dev)
{
  /* TODO: Don't hardcode baud rate. 115200 is not right for GPS */
  UART16550(dev).LCR = 0x83;
  UART16550(dev).RTX = 0x0a;
  UART16550(dev).IER = 0;
  UART16550(dev).LCR = 0x3;
}
