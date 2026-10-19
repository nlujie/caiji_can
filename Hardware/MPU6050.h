#ifndef __MPU6050_H
#define __MPU6050_H


//#include "stm32f10x.h"

// ADS1115配置
#define ADS1115_ADDR1 0x48  // 第一个ADS1115地址
#define ADS1115_ADDR2 0x49  // 第二个ADS1115地址
#define ADS1115_GAIN  0x02  // ±4.096V量程
#define ADS1115_DR    0x04  // 128SPS数据速率

// 通道定义
typedef enum {
    ADS1115_CH0 = 0,
    ADS1115_CH1,
    ADS1115_CH2,
    ADS1115_CH3
} ADS1115_Channel;

// 初始化函数
void ADS1115_Dual_Init(void);

// 采样函数
uint8_t ADS1115_StartConversion(uint8_t i2c_num, ADS1115_Channel ch);
int16_t ADS1115_GetConversionResult(uint8_t i2c_num);
uint8_t ADS1115_IsConversionComplete(uint8_t i2c_num);

// 直接获取电压值
float ADS1115_GetVoltage(uint8_t i2c_num, ADS1115_Channel ch);

#endif








