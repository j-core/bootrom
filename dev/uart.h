#ifndef UART_H
#define UART_H

//==============================
// Send Tx
// -----------------------------
//     Input  : dev = device index, data = send data
//     Output : none
//==============================
void uart_tx (int dev, unsigned char data);

//====================================
// Receive RX
// -----------------------------------
//     Input  : dev = device index
//     Output : uart_rx = receive data
//====================================
unsigned char uart_rx (int dev);

//====================================
// Receive RX without blocking
// -----------------------------------
//     Input  : dev = device index
//              c = where received char            
//     Output : 0 if success, -1 if nothing received
//====================================
int uart_rx_no_block (int dev, unsigned char *c);

//=========================================
// Receive RX with echo to TX
// ----------------------------------------
//     Input  : dev = device index
//     Output : uart_rx_echo = receive data
//=========================================
unsigned char uart_rx_echo (int dev);

//==================
// Flush RXD FIFO
//------------------
//     Input  : dev = device index
//     Output : none
//==================
void uart_rx_flush (int dev);

//==============================
// Set Baud Rate
//------------------------------
//     Input  : dev = device index
//     Output : none
//==============================
void uart_set_baudrate (int dev);


#endif
