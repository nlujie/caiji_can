#ifndef __FUNC_H
#define __FUNC_H

#include "stm32f10x.h"
#include "filter.h"  // 包含滤波头文件

// 定义滤波结构体
typedef struct {
    uint32_t time_1ms;
} Task_Timer;

// CAN消息结构
typedef struct {
    uint32_t id;
    uint8_t IDE;
    uint8_t RTR;
    uint8_t len;
    uint8_t data[8];
} CAN_Message;

// ADC原始数据结构体 - 每个芯片4个通道
typedef struct {
    uint16_t ain0;  // 通道0原始ADC值
    uint16_t ain1;  // 通道1原始ADC值
    uint16_t ain2;  // 通道2原始ADC值
    uint16_t ain3;  // 通道3原始ADC值
} ADC_Raw_Data_t;

// CAN数据联合体 - 用于在结构体和字节数组之间转换
typedef union {
    ADC_Raw_Data_t channels;  // 以结构体形式访问
    uint8_t bytes[8];         // 以字节数组形式访问
    uint16_t words[4];        // 以16位数组形式访问
} CAN_Data_Union_t;

// 传感器类型枚举
typedef enum {
    SENSOR_4_20MA = 0,    // 4-20mA电流传感器
    SENSOR_THERMISTOR,    // 热电阻温度传感器
    SENSOR_VOLTAGE_0_10V  // 0-10V电压传感器
} Sensor_Type_t;

// 通道配置结构体
typedef struct {
    Sensor_Type_t type;   // 传感器类型
    const char* name;     // 通道名称
} Channel_Config_t;

// 芯片配置结构体
typedef struct {
    const char* chip_name;           // 芯片名称
    Channel_Config_t channels[4];    // 4个通道的配置
} Chip_Config_t;

// 声明全局变量
extern Task_Timer task_tim1;
extern CAN_Message can_rx_msg;

// SPI1第一个芯片 (PA4片选) - 全局变量
extern float spi1_chip1_ain0_current;
extern float spi1_chip1_ain1_current;  
extern float spi1_chip1_ain2_temp;
extern float spi1_chip1_ain3_temp;

// SPI1第二个芯片 (PA8片选) - 全局变量
extern float spi1_chip2_ain0_current;
extern float spi1_chip2_ain1_current;
extern float spi1_chip2_ain2_current;
extern float spi1_chip2_ain3_temp;

// SPI2芯片 - 全局变量
extern float spi2_ain0_voltage;
extern float spi2_ain1_current;
extern float spi2_ain2_current;
extern float spi2_ain3_temp;

// 原始ADC数据存储（使用结构体）
extern ADC_Raw_Data_t raw_adc_spi1_chip1;
extern ADC_Raw_Data_t raw_adc_spi1_chip2;
extern ADC_Raw_Data_t raw_adc_spi2;

// 复合滤波结构体
extern CompositeFilter comp_filter_spi1_chip1_ain0;
extern CompositeFilter comp_filter_spi1_chip1_ain1;
extern CompositeFilter comp_filter_spi1_chip1_ain2;
extern CompositeFilter comp_filter_spi1_chip1_ain3;
extern CompositeFilter comp_filter_spi1_chip2_ain0;
extern CompositeFilter comp_filter_spi1_chip2_ain1;
extern CompositeFilter comp_filter_spi1_chip2_ain2;
extern CompositeFilter comp_filter_spi1_chip2_ain3;
extern CompositeFilter comp_filter_spi2_ain0;
extern CompositeFilter comp_filter_spi2_ain1;
extern CompositeFilter comp_filter_spi2_ain2;
extern CompositeFilter comp_filter_spi2_ain3;

// 滤波后的ADC值
extern uint16_t filtered_adc_spi1_chip1_ain0;
extern uint16_t filtered_adc_spi1_chip1_ain1;
extern uint16_t filtered_adc_spi1_chip1_ain2;
extern uint16_t filtered_adc_spi1_chip1_ain3;
extern uint16_t filtered_adc_spi1_chip2_ain0;
extern uint16_t filtered_adc_spi1_chip2_ain1;
extern uint16_t filtered_adc_spi1_chip2_ain2;
extern uint16_t filtered_adc_spi1_chip2_ain3;
extern uint16_t filtered_adc_spi2_ain0;
extern uint16_t filtered_adc_spi2_ain1;
extern uint16_t filtered_adc_spi2_ain2;
extern uint16_t filtered_adc_spi2_ain3;

// CAN ID定义
#define CAN_ID_SPI1_CHIP1_RAW  0x110  // SPI1第一个芯片原始数据
#define CAN_ID_SPI1_CHIP2_RAW  0x111  // SPI1第二个芯片原始数据
#define CAN_ID_SPI2_RAW        0x112  // SPI2芯片原始数据



//#define CAN_ID_HTM440_RAW       0x303  // 温湿度原始整数数据


// 温湿度原始数据联合体
typedef union {
    struct {
        int16_t raw_temp;      // 原始温度值（实际值×10）
        int16_t raw_humidity;  // 原始湿度值（实际值×10）
        uint8_t device_addr;   // 设备地址
        uint8_t status;        // 状态标志
        uint8_t reserved[2];   // 保留字节
    } data;
    uint8_t bytes[8];
} HTM440_Raw_Data_Union_t;



// 函数声明
void IWDG_Init(uint32_t timeout_ms);
void Check_IWDG_Reset(void);
void CAN_Rx_Callback(CanRxMsg* rx_msg);
void CAN_Module_Init(void);
void LED1_Init(void);
void LED1_Process(void);
void TIM3_Int_Init(uint16_t arr, uint16_t psc);
void Control_Task(void);
void task_1ms(void);
void task_2ms(void);
void task_5ms(void);
void task_10ms(void);
void task_20ms(void);
void task_50ms(void);
void task_100ms(void);
void task_200ms(void);

// 传感器相关函数
void Read_ADS1118_Direct(void);
float ConvertVoltageToCurrent_2V(float voltage_v);
float ConvertVoltageToOriginal_10V(float voltage_v);
float CalculateNTCResistance(float voltage_v);
float CalculateNTCTemperature(float resistance);

// CAN发送函数
void CAN_Send_ADC_Raw_Data(uint32_t can_id, CAN_Data_Union_t* can_data);
void CAN_Send_All_ADC_Raw_Data(void);
const char* Get_Sensor_Type_Name(Sensor_Type_t type);

////// 温湿度CAN发送函数
////void CAN_Send_HTM440_Raw_Data(void);

void Serial_SendFloat(float value, int decimal_places);

// 初始化函数
void Initialize_Composite_Filter_Structs(void);

void HTM440_Init(void);
void HTM440_SetReadInterval(uint32_t interval_ms);

#endif

