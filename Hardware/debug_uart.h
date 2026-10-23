#ifndef __DEBUG_UART_H
#define __DEBUG_UART_H

#include "stm32f10x.h"

/*
 * Debug output uses USART1 TX on PA9, 115200 baud, 8 data bits,
 * no parity, 1 stop bit. Connect PA9 -> USB-UART RX and share GND.
 */
void DebugUart_Init(void);
int DebugUart_Printf(const char *format, ...);

#endif
