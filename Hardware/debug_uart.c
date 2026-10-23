#include "debug_uart.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include <stdio.h>
#include <stdarg.h>

#define DEBUG_UART_BAUDRATE 115200U
#define DEBUG_UART_BUFFER_SIZE 256U

void DebugUart_Init(void)
{
    GPIO_InitTypeDef gpio;
    USART_InitTypeDef usart;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);

    /* USART1_TX = PA9. PA10/RX is deliberately unused for debug-only output. */
    gpio.GPIO_Pin = GPIO_Pin_9;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &gpio);

    usart.USART_BaudRate = DEBUG_UART_BAUDRATE;
    usart.USART_WordLength = USART_WordLength_8b;
    usart.USART_StopBits = USART_StopBits_1;
    usart.USART_Parity = USART_Parity_No;
    usart.USART_Mode = USART_Mode_Tx;
    usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_Init(USART1, &usart);
    USART_Cmd(USART1, ENABLE);
}

int DebugUart_Printf(const char *format, ...)
{
    char buffer[DEBUG_UART_BUFFER_SIZE];
    va_list args;
    int length;
    int i;

    if (format == 0)
    {
        return -1;
    }

    va_start(args, format);
    length = vsnprintf(buffer, DEBUG_UART_BUFFER_SIZE, format, args);
    va_end(args);

    if (length < 0)
    {
        return length;
    }

    if ((unsigned int)length >= DEBUG_UART_BUFFER_SIZE)
    {
        length = (int)DEBUG_UART_BUFFER_SIZE - 1;
    }

    for (i = 0; i < length; i++)
    {
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET)
        {
        }
        USART_SendData(USART1, (uint16_t)(uint8_t)buffer[i]);
    }

    /* Ensure this line is fully out before the caller changes the buffer. */
    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET)
    {
    }

    return length;
}
