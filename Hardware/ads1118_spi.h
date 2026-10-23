#ifndef __ADS1118_SPI_H
#define __ADS1118_SPI_H

#include "stm32f10x.h"

/* ADS1118: single-shot, single-ended AINx-GND, +/-2.048 V,
 * 128 SPS, internal voltage reference, valid config write (NOP=01).
 * 1 LSB = 62.5 uV. 100 ohm shunt: 4-20 mA -> 0.4-2.0 V.
 */
#define ADS1118_CONFIG_AIN0  ((u16)0xC58B)
#define ADS1118_CONFIG_AIN1  ((u16)0xD58B)
#define ADS1118_CONFIG_AIN2  ((u16)0xE58B)
#define ADS1118_CONFIG_AIN3  ((u16)0xF58B)

/* Schematic: all three ADS1118 devices share SPI1.
 * PB0: N_O1 (P1/P2/P5/P4)
 * PB1: N_O2 (actuator feedback AIN0/1, P10/P11 AIN2/3)
 * PB10: N_O3 (P6/P7/P9/P8)
 */
#define ADC_CS1_Low()   GPIO_ResetBits(GPIOB, GPIO_Pin_0)
#define ADC_CS1_High()  GPIO_SetBits(GPIOB, GPIO_Pin_0)
#define ADC_CS2_Low()   GPIO_ResetBits(GPIOB, GPIO_Pin_1)
#define ADC_CS2_High()  GPIO_SetBits(GPIOB, GPIO_Pin_1)
#define ADC_CS3_Low()   GPIO_ResetBits(GPIOB, GPIO_Pin_10)
#define ADC_CS3_High()  GPIO_SetBits(GPIOB, GPIO_Pin_10)

extern volatile u32 ads1118_spi_timeout_count;
extern volatile u32 ads1118_conversion_count;

void SPI1_Init(void);
void SPI2_Init(void);             /* legacy API, not used for board ADC */
void ADS1118_Init(void);
void SPI1_SetSpeed(u8 SPI_BaudRatePrescaler);
void SPI2_SetSpeed(u8 SPI_BaudRatePrescaler);
u8 ADS1118_ReadWrite_SPI1(u8 ConfigReg1);
u8 ADS1118_ReadWrite_SPI2(u8 ConfigReg1);
u16 ADS1118_ReadVotage_SPI1_Chip1(u16 ConfigReg);
u16 ADS1118_ReadVotage_SPI1_Chip2(u16 ConfigReg);
u16 ADS1118_ReadVotage_SPI2(u16 ConfigReg); /* legacy name: third SPI1 ADS1118, CS=PB10 */
u16 ADS1118_ReadVotage_SPI1_Chip1_Optimized(u16 ConfigReg);
u16 ADS1118_ReadVotage_SPI1_Chip2_Optimized(u16 ConfigReg);

#endif
