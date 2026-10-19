#ifndef __SERIAL1_H
#define __SERIAL1_H
#include <stdio.h>

void Serial_Init_1();
void Serial_SendByte_1(uint8_t Byte);
void Serial_SendArray_1(uint8_t *array, uint16_t length);
void Serial_SendString_1(char *string);
void Serial_pow_1(uint32_t x,uint32_t y);
void Serial_SendNum_1(uint32_t Num,uint16_t length);
int fputc_1(int ch, FILE *f);

uint8_t Serial_GetRxFlag_1(void);


void Serial_SendHexByte_1(uint8_t byte);
void Serial_SendHex_1(uint16_t num);

#endif
