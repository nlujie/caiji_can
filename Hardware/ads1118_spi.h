#ifndef __ADS1118_SPI_H
#define __ADS1118_SPI_H

#include "stm32f10x.h"

// 4 single-ended inputs to GND, +/-2.048V PGA, 860SPS continuous.
// PCB 4-20mA gives 0.4-2.0V across nominal 100-ohm resistor.
#define ADS1118_CONFIG_AIN0 0x44EBu
#define ADS1118_CONFIG_AIN1 0x54EBu
#define ADS1118_CONFIG_AIN2 0x64EBu
#define ADS1118_CONFIG_AIN3 0x74EBu

void SPI1_Init(void);
void SPI2_Init(void);
void ADS1118_Init(void);
void SPI1_SetSpeed(u8 SPI_BaudRatePrescaler);
void SPI2_SetSpeed(u8 SPI_BaudRatePrescaler);
u8 ADS1118_ReadWrite_SPI1(u8 ConfigReg1);
u8 ADS1118_ReadWrite_SPI2(u8 ConfigReg1);
u16 ADS1118_ReadVotage_SPI1_Chip1(u16 ConfigReg);
u16 ADS1118_ReadVotage_SPI1_Chip2(u16 ConfigReg);
u16 ADS1118_ReadVotage_SPI2(u16 ConfigReg);


u16 ADS1118_ReadVotage_SPI1_Chip1_Optimized(u16 ConfigReg);
u16 ADS1118_ReadVotage_SPI1_Chip2_Optimized(u16 ConfigReg);

// CS왠齡브
#define ADC_CS1_Low()  GPIO_ResetBits(GPIOA, GPIO_Pin_4)   // SPI1뒤寧몸棘튬
#define ADC_CS1_High() GPIO_SetBits(GPIOA, GPIO_Pin_4)
#define ADC_CS2_Low()  GPIO_ResetBits(GPIOA, GPIO_Pin_3)   // SPI1뒤랗몸棘튬
#define ADC_CS2_High() GPIO_SetBits(GPIOA, GPIO_Pin_3)
#define ADC_CS3_Low()  GPIO_ResetBits(GPIOB, GPIO_Pin_12)  // SPI2棘튬
#define ADC_CS3_High() GPIO_SetBits(GPIOB, GPIO_Pin_12)

#endif
