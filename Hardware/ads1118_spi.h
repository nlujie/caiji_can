#ifndef __ADS1118_SPI_H
#define __ADS1118_SPI_H

#include "stm32f10x.h"

/* ADS1118: PGA[2:0]=010 => ±2.048V满量程，16bit有符号值，1LSB=62.5uV。
 * 此处配置的±2.048V档位不代表20mA对应2.048V；应由采样电阻决定。
 */
#define ADS1118_CONFIG_AIN0 0xC5EB // AIN0데똥渴흙, ÷2.048V   0100 0100 1110 
//#define ADS1118_CONFIG_AIN0 0xC5EB

#define ADS1118_CONFIG_AIN1 0xD5EB // AIN1데똥渴흙, ÷2.048V  
#define ADS1118_CONFIG_AIN2 0xE5EB // AIN2데똥渴흙, ÷2.048V
#define ADS1118_CONFIG_AIN3 0xF5EB // AIN3데똥渴흙, ÷2.048V

//#define ADS1118_CONFIG_AIN0 0xC58B // AIN0데똥渴흙, ÷2.048V   0100 0100 1110 
//#define ADS1118_CONFIG_AIN1 0xD58B // AIN1데똥渴흙, ÷2.048V  
//#define ADS1118_CONFIG_AIN2 0xE58B// AIN2데똥渴흙, ÷2.048V
//#define ADS1118_CONFIG_AIN3 0xF58B // AIN3데똥渴흙, ÷2.048V


// 변鑒츠
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
/* chip: 1=N_O1/PB0, 2=N_O2/PB1, 3=N_O3/PB10; channel: AIN0..AIN3 */
u16 ADS1118_ReadChannel_SPI1(u8 chip, u8 channel);
/* 1=SPI/config echo OK; 0=SPI error. chip=1/2/3, channel=0..3. */
u8 ADS1118_ReadChannelChecked_SPI1(u8 chip, u8 channel,
                                   u16 *raw, u16 *last_config_echo);


u16 ADS1118_ReadVotage_SPI1_Chip1_Optimized(u16 ConfigReg);
u16 ADS1118_ReadVotage_SPI1_Chip2_Optimized(u16 ConfigReg);

// CS왠齡브
#define ADC_CS1_Low()  GPIO_ResetBits(GPIOB, GPIO_Pin_0)   // N_O1，SPI1片选
#define ADC_CS1_High() GPIO_SetBits(GPIOB, GPIO_Pin_0)
#define ADC_CS2_Low()  GPIO_ResetBits(GPIOB, GPIO_Pin_1)   // N_O2，SPI1片选
#define ADC_CS2_High() GPIO_SetBits(GPIOB, GPIO_Pin_1)
#define ADC_CS3_Low()  GPIO_ResetBits(GPIOB, GPIO_Pin_10)  // N_O3，SPI1片选
#define ADC_CS3_High() GPIO_SetBits(GPIOB, GPIO_Pin_10)

#endif
