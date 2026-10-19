#ifndef __SERIAL2_H
#define __SERIAL2_H
#include <stdio.h>

void Serial_Init_2();
void Serial_SendByte_2(uint8_t Byte);
void Serial_SendArray_2(uint8_t *array, uint16_t length);
void Serial_SendString_2(const char *string);
void Serial_pow_2(uint32_t x,uint32_t y);
void Serial_SendNum_2(uint32_t Num,uint16_t length);
int fputc_2(int ch, FILE *f);

uint8_t Serial_GetRxFlag_2(void);


uint8_t Serial_SendBuffer_DMA_2(uint8_t *buffer, uint16_t length);

void Serial_SendHexByte_2(uint8_t byte);
void Serial_SendHex_2(uint16_t num);
void Modbus_SendResponse_DMA(uint8_t *data, uint16_t length);
void Serial_SendNumber_2(int32_t number);

#endif



