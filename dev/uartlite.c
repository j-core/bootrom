/* Simple Xilinx uartlite compatible UART routines */

#include "uart.h"
#include "board.h"

static volatile struct uartlite_regs *devices[] = {
#ifdef DEVICE_UART0
  DEVICE_UART0,
#elif defined(DEVICE_UART)
  DEVICE_UART,
#else
  0,
#endif
#ifdef DEVICE_UART1
  DEVICE_UART1,
#else
  0,
#endif
#ifdef DEVICE_UARTGPS
  DEVICE_UARTGPS
#else
  0
#endif
};

void
uart_tx (int dev, unsigned char data)
{
  if (devices[dev] == 0) return;
  while (devices[dev]->status & 0x08);
  devices[dev]->tx = data;
}

unsigned char
uart_rx (int dev)
{
  if (devices[dev] == 0) return 0;
  while (!(devices[dev]->status & 0x01));
  return (devices[dev]->rx & 0xff);
}

int uart_rx_no_block (int dev, unsigned char *c) {
  if (devices[dev] == 0) return -1;
  if ((devices[dev]->status & 0x01)) {
    *c = (devices[dev]->rx & 0xff);
    return 0;
  }
  return -1;
}

unsigned char
uart_rx_echo (int dev)
{
  unsigned char data;
  if (devices[dev] == 0) return 0;
  uart_tx(dev, data = uart_rx(dev));
  return data;
}

void
uart_rx_flush (int dev)
{
  if (devices[dev] == 0) return;
  devices[dev]->ctrl = 0x02;
}

void
uart_set_baudrate (int dev)
{
   /* baud is fixed in a VHDL generic */
}
