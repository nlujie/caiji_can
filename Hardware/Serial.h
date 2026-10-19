/*
#ifndef __SERIAL_H
#define __SERIAL_H
#include <stdio.h>

void Serial_Init();
void Serial_SendByte(uint8_t Byte);
void Serial_SendArray(uint8_t *array, uint16_t length);
void Serial_SendString(char *string);
void Serial_pow(uint32_t x,uint32_t y);
void Serial_SendNum(uint32_t Num,uint16_t length);
int fputc(int ch, FILE *f);

uint8_t Serial_GetRxFlag(void);

#endif
*/
#ifndef __SERIAL_H
#define __SERIAL_H
#include <stdio.h>

void Serial_Init();
void Serial_SendByte(uint8_t Byte);
void Serial_SendNumber(int32_t number);
void Serial_SendArray(uint8_t *array, uint16_t length);
void Serial_SendString(char *string);
void Serial_pow(uint32_t x,uint32_t y);
void Serial_SendNum(uint32_t Num,uint16_t length);
int fputc(int ch, FILE *f);

uint8_t Serial_GetRxFlag(void);

void Serial_SendHexByte(uint8_t byte);
void Serial_SendHex(uint16_t num);

uint8_t Serial_SendBuffer_DMA(uint8_t *buffer, uint16_t length);

#endif
