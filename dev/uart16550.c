/* Simple 16550 compatible UART routines */

#include "uart.h"
#include "board.h"

static volatile struct uart16550_regs *devices[] = {
#ifdef DEVICE_UART0
  DEVICE_UART0,
#else
  0,
#endif
#ifdef DEVICE_UART1
  DEVICE_UART1,
#else
  0,
#endif
#ifdef DEVICE_UARTGPS
  DEVICE_UARTGPS,
#else
  0
#endif
};

void
uart_tx (int dev, unsigned char data)
{
  if (devices[dev] == 0) return;
  while (!((devices[dev]->lsr) & 0x20));
  devices[dev]->rtx = data;
}

unsigned char
uart_rx (int dev)
{
  if (devices[dev] == 0) return 0;
  while (!((devices[dev]->lsr) & 0x1));
  return (devices[dev]->rtx & 0xff);
}

int uart_rx_no_block (int dev, unsigned char *c) {
  if (devices[dev] == 0) return -1;
  if (((devices[dev]->lsr) & 0x1)) {
    *c = (devices[dev]->rtx & 0xff);
    return 0;
  }
  return -1;
}

unsigned char
uart_rx_echo (int dev)
{
  unsigned char data;
  if (devices[dev] == 0) return 0;

  while (!(devices[dev]->lsr & 1));
  data = devices[dev]->rtx & 0xff;

  while (!(devices[dev]->lsr & 0x20));
  devices[dev]->rtx = data;
  return data;
}

void
uart_rx_flush (int dev)
{
  if (devices[dev] == 0) return;
  while (devices[dev]->lsr & 1) devices[dev]->rtx;
}

void
uart_set_baudrate (int dev)
{
  if (devices[dev] == 0) return;
  /* TODO: Don't hardcode baud rate. 115200 is not right for GPS */
  devices[dev]->lcr = 0x83;
  devices[dev]->rtx = 0x0a;
  devices[dev]->ier = 0;
  devices[dev]->lcr = 0x3;
}
