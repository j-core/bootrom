/* Simple Xilinx uartlite compatible UART routines */

#include "uart.h"
#include "board.h"

struct st_uartlite
{
  unsigned int rxfifo;
  unsigned int txfifo;
  unsigned int status;
  unsigned int control;
};

static const struct st_uartlite* devices[] = {
  (struct st_uartlite *) sys_UART0_BASE,
  (struct st_uartlite *) sys_UART1_BASE,
  (struct st_uartlite *) sys_GPS_BASE
};

#define UARTLITE(n) (*(volatile struct st_uartlite *) devices[n])

void
uart_tx (int dev, unsigned char data)
{
  while (UARTLITE(dev).status & 0x08);
  UARTLITE(dev).txfifo = data;
}

unsigned char
uart_rx (int dev)
{
  while (!(UARTLITE(dev).status & 0x01));
  return (UARTLITE(dev).rxfifo & 0xff);
}

int uart_rx_no_block (int dev, unsigned char *c) {
  if ((UARTLITE(dev).status & 0x01)) {
    *c = (UARTLITE(dev).rxfifo & 0xff);
    return 0;
  }
  return -1;
}

unsigned char
uart_rx_echo (int dev)
{
  unsigned char data;

  uart_tx(dev, data = uart_rx(dev));

  return data;
}

void
uart_rx_flush (int dev)
{
  UARTLITE(dev).control = 0x02;
}

void
uart_set_baudrate (int dev)
{
   /* baud is fixed in a VHDL generic */
}
