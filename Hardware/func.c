#include "func.h"
#include "stm32f10x.h"
#include "can.h"
#include "string.h"
#include "serial.h"
#include "serial2.h"
#include "ads1118_spi.h"
#include <stdio.h>
#include "Delay.h"
#include "modbus_rtu_slave.h"
#include <math.h>
#include "filter.h"
#include "modbus_rtu_master.h"

// 全局变量定义
Task_Timer task_tim1 = {0};
CAN_Message can_rx_msg = {0, 0, 0, 0, {0}};


// 在func.c的全局变量区域添加
// 温湿度读取相关变量
extern HTM440_Data_t htm440_data;
uint32_t htm440_read_interval = 2000;  // 默认2秒读取一次
uint32_t last_read_attempt = 0;
uint32_t last_htm440_read_time = 0;
uint8_t htm440_enabled = 1;  // 温湿度读取使能标志
uint8_t htm440_slave_addr = 0x01;  // 默认从站地址


// 复合滤波结构体定义
// SPI1第一个芯片复合滤波
CompositeFilter comp_filter_spi1_chip1_ain0 = {0};
CompositeFilter comp_filter_spi1_chip1_ain1 = {0};
CompositeFilter comp_filter_spi1_chip1_ain2 = {0};
CompositeFilter comp_filter_spi1_chip1_ain3 = {0};

// SPI1第二个芯片复合滤波
CompositeFilter comp_filter_spi1_chip2_ain0 = {0};
CompositeFilter comp_filter_spi1_chip2_ain1 = {0};
CompositeFilter comp_filter_spi1_chip2_ain2 = {0};
CompositeFilter comp_filter_spi1_chip2_ain3 = {0};

// SPI2芯片复合滤波
CompositeFilter comp_filter_spi2_ain0 = {0};
CompositeFilter comp_filter_spi2_ain1 = {0};
CompositeFilter comp_filter_spi2_ain2 = {0};
CompositeFilter comp_filter_spi2_ain3 = {0};

// 滤波后的ADC值
uint16_t filtered_adc_spi1_chip1_ain0 = 0;
uint16_t filtered_adc_spi1_chip1_ain1 = 0;
uint16_t filtered_adc_spi1_chip1_ain2 = 0;
uint16_t filtered_adc_spi1_chip1_ain3 = 0;

uint16_t filtered_adc_spi1_chip2_ain0 = 0;
uint16_t filtered_adc_spi1_chip2_ain1 = 0;
uint16_t filtered_adc_spi1_chip2_ain2 = 0;
uint16_t filtered_adc_spi1_chip2_ain3 = 0;

uint16_t filtered_adc_spi2_ain0 = 0;
uint16_t filtered_adc_spi2_ain1 = 0;
uint16_t filtered_adc_spi2_ain2 = 0;
uint16_t filtered_adc_spi2_ain3 = 0;// 滤波结构体定义
//// SPI1第一个芯片滤波
//FilterStruct_Float filter_spi1_chip1_ain0 = {0};
//FilterStruct_Float filter_spi1_chip1_ain1 = {0};
//FilterStruct_Float filter_spi1_chip1_ain2 = {0};
//FilterStruct_Float filter_spi1_chip1_ain3 = {0};

//// SPI1第二个芯片滤波
//FilterStruct_Float filter_spi1_chip2_ain0 = {0};
//FilterStruct_Float filter_spi1_chip2_ain1 = {0};
//FilterStruct_Float filter_spi1_chip2_ain2 = {0};
//FilterStruct_Float filter_spi1_chip2_ain3 = {0};

//// SPI2芯片滤波
//FilterStruct_Float filter_spi2_ain0 = {0};
//FilterStruct_Float filter_spi2_ain1 = {0};
//FilterStruct_Float filter_spi2_ain2 = {0};
//FilterStruct_Float filter_spi2_ain3 = {0};

//// 滤波后的ADC值
//uint16_t filtered_adc_spi1_chip1_ain0 = 0;
//uint16_t filtered_adc_spi1_chip1_ain1 = 0;
//uint16_t filtered_adc_spi1_chip1_ain2 = 0;
//uint16_t filtered_adc_spi1_chip1_ain3 = 0;

//uint16_t filtered_adc_spi1_chip2_ain0 = 0;
//uint16_t filtered_adc_spi1_chip2_ain1 = 0;
//uint16_t filtered_adc_spi1_chip2_ain2 = 0;
//uint16_t filtered_adc_spi1_chip2_ain3 = 0;

//uint16_t filtered_adc_spi2_ain0 = 0;
//uint16_t filtered_adc_spi2_ain1 = 0;
//uint16_t filtered_adc_spi2_ain2 = 0;
//uint16_t filtered_adc_spi2_ain3 = 0;



// SPI1第一个芯片 (PA4片选)
float spi1_chip1_ain0_current = 0.0f;
float spi1_chip1_ain1_current = 0.0f;  
float spi1_chip1_ain2_temp = 0.0f;
float spi1_chip1_ain3_temp = 0.0f;

// SPI1第二个芯片 (PA8片选)
float spi1_chip2_ain0_current = 0.0f;
float spi1_chip2_ain1_current = 0.0f;
float spi1_chip2_ain2_current = 0.0f;
float spi1_chip2_ain3_temp = 0.0f;

// SPI2芯片
float spi2_ain0_voltage = 0.0f;
float spi2_ain1_current = 0.0f;
float spi2_ain2_current = 0.0f;
float spi2_ain3_temp = 0.0f;

// 原始ADC数据存储（使用结构体）
ADC_Raw_Data_t raw_adc_spi1_chip1 = {0};
ADC_Raw_Data_t raw_adc_spi1_chip2 = {0};
ADC_Raw_Data_t raw_adc_spi2 = {0};

// 芯片配置表
const Chip_Config_t chip_configs[] = {
    // SPI1第一个芯片配置 (PA4)
    {
        "SPI1-Chip1(PA4)",
        {
            {SENSOR_4_20MA, "4-20mA"},     // AIN0
            {SENSOR_4_20MA, "4-20mA"},     // AIN1
            {SENSOR_THERMISTOR, "Temp"},   // AIN2
            {SENSOR_THERMISTOR, "Temp"}    // AIN3
        }
    },
    // SPI1第二个芯片配置 (PA8)
    {
        "SPI1-Chip2(PA8)",
        {
            {SENSOR_4_20MA, "4-20mA"},     // AIN0
            {SENSOR_4_20MA, "4-20mA"},     // AIN1
            {SENSOR_4_20MA, "4-20mA"},     // AIN2
            {SENSOR_THERMISTOR, "Temp"}    // AIN3
        }
    },
    // SPI2芯片配置 (PB12)
    {
        "SPI2-Chip(PB12)",
        {
            {SENSOR_VOLTAGE_0_10V, "0-10V"}, // AIN0
            {SENSOR_4_20MA, "4-20mA"},       // AIN1
            {SENSOR_4_20MA, "4-20mA"},       // AIN2
            {SENSOR_THERMISTOR, "Temp"}      // AIN3
        }
    }
};


// 自定义串口发送浮点数函数
void Serial_SendFloat(float value, int decimal_places) {
   //// char buffer[20];
    
    // 处理负数
    if(value < 0) {
        Serial_SendString("-");
        value = -value;
    }
    
    // 提取整数部分
    int integer_part = (int)value;
    
    // 提取小数部分
    float fractional_part = value - integer_part;
    int fractional_int = 0;
    
    // 计算小数部分为整数
    for(int i = 0; i < decimal_places; i++) {
        fractional_part *= 10;
    }
    fractional_int = (int)fractional_part;
    
    // 发送整数部分
    char int_str[10];
    sprintf(int_str, "%d", integer_part);
    Serial_SendString(int_str);
    
    // 发送小数点
    Serial_SendString(".");
    
    // 发送小数部分，补零
    char frac_str[10];
    sprintf(frac_str, "%0*d", decimal_places, fractional_int);
    Serial_SendString(frac_str);
}




// 参数定义
#define SHUNT_RESISTOR_100OHM     100.0f
#define VOLTAGE_DIVIDER_RATIO     (10.0f / 2.048f)
#define ADC_CONVERSION_FACTOR     0.0625f

// NTC参数
#define NTC_R25 10000.0f
#define NTC_BETA 3435.0f
#define NTC_T25 298.15f
#define REF_RESISTOR 510.0f
#define NTC_EXCITATION_VOLTAGE 2.048f
#define NTC_EXCITATION_VOLTAGE_t1 2.048f
#define NTC_EXCITATION_VOLTAGE_t2 2.048f
#define NTC_EXCITATION_VOLTAGE_t3 2.048f

// CAN活动跟踪
uint32_t last_can_activity_time = 0;
#define CAN_ACTIVITY_TIMEOUT  1000

// 获取传感器类型名称
const char* Get_Sensor_Type_Name(Sensor_Type_t type) {
    switch(type) {
        case SENSOR_4_20MA: return "4-20mA";
        case SENSOR_THERMISTOR: return "Thermistor";
        case SENSOR_VOLTAGE_0_10V: return "0-10V";
        default: return "Unknown";
    }
}

// 独立看门狗初始化
//void IWDG_Init(uint32_t timeout_ms) {
//    RCC_LSICmd(ENABLE);
//    while(RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET);
//    
//    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
//    IWDG_SetPrescaler(IWDG_Prescaler_64);
//    
//    uint16_t reload_value = (2000 * 625) / 1000;
//    if(reload_value > 0xFFF) reload_value = 0xFFF;
//    
//    IWDG_SetReload(reload_value);
//    IWDG_ReloadCounter();
//    IWDG_Enable();
//    
//    Serial_SendString_2("IWDG Initialized - Timeout: 2s\r\n");
//}

//void Check_IWDG_Reset(void) {
//    if(RCC_GetFlagStatus(RCC_FLAG_IWDGRST) != RESET) {
//        Serial_SendString_2("*** System was reset by IWDG! ***\r\n");
//        RCC_ClearFlag();
//    }
//}

// CAN接收回调
void CAN_Rx_Callback(CanRxMsg* rx_msg) {
    can_rx_msg.id = rx_msg->StdId;
    can_rx_msg.IDE = rx_msg->IDE;
    can_rx_msg.RTR = rx_msg->RTR;
    can_rx_msg.len = rx_msg->DLC;
    for(int i=0; i<rx_msg->DLC; i++)
        can_rx_msg.data[i] = rx_msg->Data[i];
    
    last_can_activity_time = task_tim1.time_1ms;
    
////    Serial_SendString("CAN Message Received: ID=0x");
////    Serial_SendHex_2(can_rx_msg.id);
////    Serial_SendString(", Data=");
    for(int i=0; i<can_rx_msg.len; i++) {
////        Serial_SendHexByte(can_rx_msg.data[i]);
////        Serial_SendString(" ");
    }
////    Serial_SendString("\r\n");
}

// CAN模块初始化
void CAN_Module_Init(void) {
    CAN_Mode_Init(CAN_SJW_1tq, CAN_BS2_8tq, CAN_BS1_9tq, 4, CAN_Mode_Normal);
    CAN_SetRxCallback(CAN_Rx_Callback);
    CAN_EnableInterrupt();
//    Serial_SendString("CAN module initialized with interrupt mode\r\n");
}

// 将电压值转换为电流值 (使用100Ω分流电阻，2V量程)
float ConvertVoltageToCurrent_2V(float voltage_v) {
    if(voltage_v < 0.4f) voltage_v = 0.4f;
    if(voltage_v > 2.0f) voltage_v = 2.0f;
    return (voltage_v / SHUNT_RESISTOR_100OHM) * 1000.0f;
}

// 将ADC电压转换为原始电压 (0-10V)
float ConvertVoltageToOriginal_10V(float voltage_v) {
    if(voltage_v < 0) voltage_v = 0;
    if(voltage_v > 2.048f) voltage_v = 2.048f;
    return voltage_v * VOLTAGE_DIVIDER_RATIO;
}

// 根据电压计算NTC电阻值
float CalculateNTCResistance(float voltage_v) {
    if (voltage_v >= NTC_EXCITATION_VOLTAGE) return 0.0f;
    return (voltage_v * REF_RESISTOR) / (NTC_EXCITATION_VOLTAGE - voltage_v);
}

// 根据电压计算NTC电阻值
float CalculateNTCResistance_t1(float voltage_v) {
    if (voltage_v >= NTC_EXCITATION_VOLTAGE_t1) return 0.0f;
    return (voltage_v * REF_RESISTOR) / (NTC_EXCITATION_VOLTAGE_t1 - voltage_v);
}

// 根据电压计算NTC电阻值
float CalculateNTCResistance_t2(float voltage_v) {
    if (voltage_v >= NTC_EXCITATION_VOLTAGE_t2) return 0.0f;
    return (voltage_v * REF_RESISTOR) / (NTC_EXCITATION_VOLTAGE_t2 - voltage_v);
}


// 根据电压计算NTC电阻值
float CalculateNTCResistance_t3(float voltage_v) {
    if (voltage_v >= NTC_EXCITATION_VOLTAGE_t3) return 0.0f;
    return (voltage_v * REF_RESISTOR) / (NTC_EXCITATION_VOLTAGE_t3 - voltage_v);
}


// 根据NTC电阻计算温度
float CalculateNTCTemperature(float resistance) {
    if (resistance <= 0.0f) return -273.15f;
    
    float steinhart;
    steinhart = resistance / NTC_R25;
    steinhart = log(steinhart);
    steinhart /= NTC_BETA;
    steinhart += 1.0 / NTC_T25;
    steinhart = 1.0 / steinhart;
    
    return steinhart - 273.15;
}

// CAN发送原始ADC数据（使用联合体）
void CAN_Send_ADC_Raw_Data(uint32_t can_id, CAN_Data_Union_t* can_data) {
    CanTxMsg tx_msg;
    
    tx_msg.StdId = can_id;        // 标准ID
    tx_msg.ExtId = can_id;        // 扩展ID
    tx_msg.IDE = CAN_Id_Standard; // 标准帧
    tx_msg.RTR = CAN_RTR_Data;    // 数据帧
    tx_msg.DLC = 8;               // 8字节数据长度
    
    // 使用联合体直接复制字节数据
    for(int i = 0; i < 8; i++) {
        tx_msg.Data[i] = can_data->bytes[i];
    }
    
    if(CAN_SendMessage(&tx_msg) == CAN_TxStatus_Ok) {
////        Serial_SendString("CAN raw ADC data sent successfully, ID=0x");
////        Serial_SendHex(can_id);
////        Serial_SendString("\r\n");
    } else {
////        Serial_SendString("CAN raw ADC data send failed, ID=0x");
////        Serial_SendHex(can_id);
////        Serial_SendString("\r\n");
    }
}


////// 发送温湿度原始数据到CAN（整数格式）
////void CAN_Send_HTM440_Raw_Data(void) {
////    CanTxMsg tx_msg;
////    HTM440_Raw_Data_Union_t raw_can_data;
////    
////    tx_msg.StdId = CAN_ID_HTM440_RAW;
////    tx_msg.ExtId = CAN_ID_HTM440_RAW;
////    tx_msg.IDE = CAN_Id_Standard;
////    tx_msg.RTR = CAN_RTR_Data;
////    tx_msg.DLC = 8;
////    
////    // 填充原始数据
////    raw_can_data.data.raw_temp = htm440_data.raw_temperature;
////    raw_can_data.data.raw_humidity = htm440_data.raw_humidity;
////    raw_can_data.data.device_addr = htm440_data.device_addr;
////    raw_can_data.data.status = htm440_data.valid ? 0x01 : 0x00;
////    raw_can_data.data.reserved[0] = 0x00;
////    raw_can_data.data.reserved[1] = 0x00;
////    
////    // 复制字节到CAN消息
////    for(int i = 0; i < 8; i++) {
////        tx_msg.Data[i] = raw_can_data.bytes[i];
////    }
////    
////    // 发送CAN消息
////    CAN_SendMessage(&tx_msg);
////    
////    // 可选：调试输出
////    // Serial_SendString("HTM440 Raw CAN: Temp_raw=");
////    // Serial_SendNumber(htm440_data.raw_temperature);
////    // Serial_SendString(", Hum_raw=");
////    // Serial_SendNumber(htm440_data.raw_humidity);
////    // Serial_SendString("\r\n");
////}


// 添加流量计算函数
float ConvertCurrentToFlow(float current_ma) {
    // 使用公式: X = (I - 3.59) / 0.041
    // 其中 I 是电流(mA)，X 是流量
    float flow = (current_ma - 3.59f) / 0.041f;
    return flow;
}


// 压力计算函数
float ConvertCurrentToPressure(float current_ma) {
    // 使用公式: P = (9I - 52) / 16
    // 其中 I 是电流(mA)，P 是压力(bar)
    float pressure = (9.0f * current_ma - 52.0f) / 16.0f;
    return pressure;
}


// 发送所有ADC原始数据
void CAN_Send_All_ADC_Raw_Data(void) {
    CAN_Data_Union_t can_data;
    
    
    // 使用复合滤波后的数据填充结构体
    raw_adc_spi1_chip1.ain0 = filtered_adc_spi1_chip1_ain0;
    raw_adc_spi1_chip1.ain1 = filtered_adc_spi1_chip1_ain1;
    raw_adc_spi1_chip1.ain2 = filtered_adc_spi1_chip1_ain2;
    raw_adc_spi1_chip1.ain3 = filtered_adc_spi1_chip1_ain3;
    
    raw_adc_spi1_chip2.ain0 = filtered_adc_spi1_chip2_ain0;
    raw_adc_spi1_chip2.ain1 = filtered_adc_spi1_chip2_ain1;
    raw_adc_spi1_chip2.ain2 = filtered_adc_spi1_chip2_ain2;
    raw_adc_spi1_chip2.ain3 = filtered_adc_spi1_chip2_ain3;
    
    raw_adc_spi2.ain0 = filtered_adc_spi2_ain0;
    raw_adc_spi2.ain1 = filtered_adc_spi2_ain1;
    raw_adc_spi2.ain2 = filtered_adc_spi2_ain2;
    raw_adc_spi2.ain3 = filtered_adc_spi2_ain3;
    
    
    
    // 发送SPI1第一个芯片的原始数据
    can_data.channels = raw_adc_spi1_chip1;
    CAN_Send_ADC_Raw_Data(CAN_ID_SPI1_CHIP1_RAW, &can_data);
    Delay_ms(2);
    
    // 发送SPI1第二个芯片的原始数据
    can_data.channels = raw_adc_spi1_chip2;
    CAN_Send_ADC_Raw_Data(CAN_ID_SPI1_CHIP2_RAW, &can_data);
    Delay_ms(2);
    
    // 发送SPI2芯片的原始数据
    can_data.channels = raw_adc_spi2;
    CAN_Send_ADC_Raw_Data(CAN_ID_SPI2_RAW, &can_data);
}
volatile uint16_t adc_value,a1,a2,a3,a4,adc_value1,adc_value2,adc_value3,adc_value4,adc_value5,adc_value6,adc_value7,adc_value8;
// 使用Read_ADS1118_Direct函数读取所有传感器数据
/* Read all three ADS1118 devices from the PCB's shared SPI1 bus.
 * Chip-select mapping is PB0=N_O1, PB1=N_O2, PB10=N_O3.
 * Keep raw and filtered Watch variables synchronized; no range gate is
 * allowed to prevent raw values/CAN frames from updating.
 */
void Read_ADS1118_Direct(void)
{
    /* N_O1 / PB0: ADC_4, ADC_5, ADC_7, ADC_6 (P1/P2/P5/P4) */
    a1 = ADS1118_ReadVotage_SPI1_Chip1(ADS1118_CONFIG_AIN0);
    Delay_ms(1);
    a2 = ADS1118_ReadVotage_SPI1_Chip1(ADS1118_CONFIG_AIN1);
    Delay_ms(1);
    a3 = ADS1118_ReadVotage_SPI1_Chip1(ADS1118_CONFIG_AIN2);
    Delay_ms(1);
    a4 = ADS1118_ReadVotage_SPI1_Chip1(ADS1118_CONFIG_AIN3);

    adc_value = a1;
    filtered_adc_spi1_chip1_ain0 = a1;
    filtered_adc_spi1_chip1_ain1 = a2;
    filtered_adc_spi1_chip1_ain2 = a3;
    filtered_adc_spi1_chip1_ain3 = a4;
    raw_adc_spi1_chip1.ain0 = a1;
    raw_adc_spi1_chip1.ain1 = a2;
    raw_adc_spi1_chip1.ain2 = a3;
    raw_adc_spi1_chip1.ain3 = a4;

    /* N_O2 / PB1: actuator feedback AIN0/1, P10/P11 AIN2/3 */
    adc_value1 = ADS1118_ReadVotage_SPI1_Chip2(ADS1118_CONFIG_AIN0);
    Delay_ms(1);
    adc_value2 = ADS1118_ReadVotage_SPI1_Chip2(ADS1118_CONFIG_AIN1);
    Delay_ms(1);
    adc_value3 = ADS1118_ReadVotage_SPI1_Chip2(ADS1118_CONFIG_AIN2);
    Delay_ms(1);
    adc_value4 = ADS1118_ReadVotage_SPI1_Chip2(ADS1118_CONFIG_AIN3);

    filtered_adc_spi1_chip2_ain0 = adc_value1;
    filtered_adc_spi1_chip2_ain1 = adc_value2;
    filtered_adc_spi1_chip2_ain2 = adc_value3;
    filtered_adc_spi1_chip2_ain3 = adc_value4;
    raw_adc_spi1_chip2.ain0 = adc_value1;
    raw_adc_spi1_chip2.ain1 = adc_value2;
    raw_adc_spi1_chip2.ain2 = adc_value3;
    raw_adc_spi1_chip2.ain3 = adc_value4;

    /* N_O3 / PB10: ADC_11, ADC_9, ADC_8, ADC_10 (P6/P7/P9/P8) */
    adc_value5 = ADS1118_ReadVotage_SPI2(ADS1118_CONFIG_AIN0);
    Delay_ms(1);
    adc_value6 = ADS1118_ReadVotage_SPI2(ADS1118_CONFIG_AIN1);
    Delay_ms(1);
    adc_value7 = ADS1118_ReadVotage_SPI2(ADS1118_CONFIG_AIN2);
    Delay_ms(1);
    adc_value8 = ADS1118_ReadVotage_SPI2(ADS1118_CONFIG_AIN3);

    filtered_adc_spi2_ain0 = adc_value5;
    filtered_adc_spi2_ain1 = adc_value6;
    filtered_adc_spi2_ain2 = adc_value7;
    filtered_adc_spi2_ain3 = adc_value8;
    raw_adc_spi2.ain0 = adc_value5;
    raw_adc_spi2.ain1 = adc_value6;
    raw_adc_spi2.ain2 = adc_value7;
    raw_adc_spi2.ain3 = adc_value8;

    /* Lightweight diagnostic output: raw codes first, then CAN frames. */
    Serial_SendString_2("ADS1118 raw N_O1=");
    Serial_SendNumber_2(a1); Serial_SendString_2(",");
    Serial_SendNumber_2(a2); Serial_SendString_2(",");
    Serial_SendNumber_2(a3); Serial_SendString_2(",");
    Serial_SendNumber_2(a4); Serial_SendString_2(" N_O2=");
    Serial_SendNumber_2(adc_value1); Serial_SendString_2(",");
    Serial_SendNumber_2(adc_value2); Serial_SendString_2(",");
    Serial_SendNumber_2(adc_value3); Serial_SendString_2(",");
    Serial_SendNumber_2(adc_value4); Serial_SendString_2(" N_O3=");
    Serial_SendNumber_2(adc_value5); Serial_SendString_2(",");
    Serial_SendNumber_2(adc_value6); Serial_SendString_2(",");
    Serial_SendNumber_2(adc_value7); Serial_SendString_2(",");
    Serial_SendNumber_2(adc_value8); Serial_SendString_2("\r\n");

    CAN_Send_All_ADC_Raw_Data();
}

//// 时序诊断函数+++++++
//void Diagnose_Timing_Issues(void) {
//    Serial_SendString_2("=== Timing Diagnosis ===\r\n");
//    
//    // 测试连续读取同一通道
//    Serial_SendString_2("Testing same channel consecutive reads:\r\n");
//    for(int i = 0; i < 3; i++) {
//        uint16_t adc_value = ADS1118_ReadVotage_SPI1_Chip1(ADS1118_CONFIG_AIN1);
//        float voltage_v = (adc_value * ADC_CONVERSION_FACTOR) / 1000.0f;
//        Serial_SendString_2("Read ");
//        Serial_SendNumber_2(i);
//        Serial_SendString_2(": ADC=0x");
//        Serial_SendHex_2(adc_value);
//        Serial_SendString_2(" -> ");
//        Serial_SendFloat(voltage_v, 4);
//        Serial_SendString_2("V\r\n");
//        Delay_ms(5);
//    }
//    
//    // 测试通道切换
//    Serial_SendString_2("Testing channel switching:\r\n");
//    for(int chan = 0; chan < 4; chan++) {
//        u16 config = ADS1118_CONFIG_AIN0 + (chan << 12);
//        uint16_t adc_value = ADS1118_ReadVotage_SPI1_Chip1(config);
//        float voltage_v = (adc_value * ADC_CONVERSION_FACTOR) / 1000.0f;
//        Serial_SendString_2("AIN");
//        Serial_SendNumber_2(chan);
//        Serial_SendString_2(": ");
//        Serial_SendFloat(voltage_v, 4);
//        Serial_SendString_2("V\r\n");
//        Delay_ms(5);
//    }
//}



////void HTM440_Task(void)
////{
////    static uint8_t retry_count = 0;
////    static ModbusMaster_Status read_status = MODBUS_MASTER_BUSY;
////    
////    if(!htm440_enabled) {
////        return;
////    }
////    
//////     // 每500ms尝试一次读取，而不是每次100ms任务都尝试
//////    if((task_tim1.time_1ms - last_read_attempt) >= 500) {
//////        last_read_attempt = task_tim1.time_1ms;
////    
////    // 检查是否到达读取时间间隔
////    if((task_tim1.time_1ms - last_htm440_read_time) >= htm440_read_interval) {
////        
////        // 调用非阻塞读取
////        ModbusMaster_Status status;
////        status = HTM440_Read_Temperature_Humidity(htm440_slave_addr, &htm440_data);
////        
////        if(status == MODBUS_MASTER_OK) {
////            // 读取成功
////            htm440_data.last_read = task_tim1.time_1ms;
////            retry_count = 0;
////            last_htm440_read_time = task_tim1.time_1ms;
////            
////            // 发送原始整数数据到CAN
////            CAN_Send_HTM440_Raw_Data();
////        }
////        else if(status == MODBUS_MASTER_BUSY) {
////            // 还在等待响应，不更新last_htm440_read_time，下次继续调用
////            return;
////        }
////        else {
////            // 读取失败
////            retry_count++;
////            last_htm440_read_time = task_tim1.time_1ms;
////            
////            // 如果连续失败多次，尝试重新检测设备地址
////            if(retry_count >= 3) {
////                uint8_t new_addr;
////                status = HTM440_Read_Device_Address(&new_addr);
////                if(status == MODBUS_MASTER_OK && new_addr != 0) {
////                    htm440_slave_addr = new_addr;
////                    htm440_data.device_addr = new_addr;
////                }
////                retry_count = 0;
////            }
////        }
////    }
//////}
////}


//// 温湿度读取任务
//// 在 func.c 中修改 HTM440_Task 函数
//void HTM440_Task(void)
//{
//    static uint8_t retry_count = 0;
//    
//    if(!htm440_enabled) {
//        return;
//    }
//    
//    // 检查是否到达读取时间间隔
//    if((task_tim1.time_1ms - last_htm440_read_time) >= htm440_read_interval) {
//        ModbusMaster_Status status;
//        
//        // 读取温湿度数据
//        status = HTM440_Read_Temperature_Humidity(htm440_slave_addr, &htm440_data);
//        
//        if(status == MODBUS_MASTER_OK && htm440_data.valid) {
//            // 设置时间戳
//            htm440_data.last_read = task_tim1.time_1ms;
//            
//            // 读取成功
//            retry_count = 0;
//            
//            // 打印温湿度数据
////            Serial_SendString_2("HTM440: 温度=");
////            Serial_SendFloat(htm440_data.temperature, 1);
////            Serial_SendString_2("C, 湿度=");
////            Serial_SendFloat(htm440_data.humidity, 1);
////            Serial_SendString_2("%\r\n");
//            
//        }
//            else {
//            // 读取失败
//            retry_count++;
////            Serial_SendString_2("HTM440读取失败，状态码: ");
////            Serial_SendNumber_2(status);
////            Serial_SendString_2(", 重试次数: ");
////            Serial_SendNumber_2(retry_count);
////            Serial_SendString_2("\r\n");
//            
//            // 如果连续失败多次，尝试重新检测设备地址
//            if(retry_count >= 3) {
////                Serial_SendString_2("尝试重新检测设备地址...\r\n");
//                uint8_t new_addr;
//                status = HTM440_Read_Device_Address(&new_addr);
//                if(status == MODBUS_MASTER_OK && new_addr != 0) {
//                    htm440_slave_addr = new_addr;
//                    htm440_data.device_addr = new_addr;
////                    Serial_SendString_2("发现新设备地址: 0x");
////                    Serial_SendHexByte_2(new_addr);
////                    Serial_SendString_2("\r\n");
//                }
//                retry_count = 0;
//            }
//        }
//        
//        // 更新最后读取时间
//        last_htm440_read_time = task_tim1.time_1ms;
//    }
//}
// 初始化温湿度传感器
void HTM440_Init(void)
{
    ModbusMaster_Init();
    
    // 尝试读取设备地址
    uint8_t addr;
    ModbusMaster_Status status = HTM440_Read_Device_Address(&addr);
    
    if(status == MODBUS_MASTER_OK && addr != 0) {
        htm440_slave_addr = addr;
        htm440_data.device_addr = addr;
//        Serial_SendString_2("HTM440设备地址: 0x");
//        Serial_SendHexByte_2(addr);
//        Serial_SendString_2("\r\n");
    } else {
//        Serial_SendString_2("使用默认设备地址: 0x");
//        Serial_SendHexByte_2(htm440_slave_addr);
//        Serial_SendString_2("\r\n");
    }
//    
    htm440_enabled = 1;
    last_htm440_read_time = task_tim1.time_1ms;
    
//    Serial_SendString_2("HTM440温湿度传感器初始化完成\r\n");
}

// 设置温湿度读取间隔（单位：ms）
void HTM440_SetReadInterval(uint32_t interval_ms)
{
    htm440_read_interval = interval_ms;
//    Serial_SendString_2("设置温湿度读取间隔: ");
//    Serial_SendNumber_2(interval_ms);
//    Serial_SendString_2("ms\r\n");
}

// 启用/禁用温湿度读取
void HTM440_Enable(uint8_t enable)
{
    htm440_enabled = enable;
//    Serial_SendString_2(enable ? "启用温湿度读取\r\n" : "禁用温湿度读取\r\n");
}

// 手动读取温湿度（可用于调试）
void HTM440_ManualRead(void)
{
    if(htm440_enabled) {
        last_htm440_read_time = 0;  // 立即触发读取
//        Serial_SendString_2("手动触发温湿度读取...\r\n");
    }
}

// 打印当前温湿度数据
void HTM440_PrintData(void)
{
    if(htm440_data.valid) {
//        Serial_SendString_2("当前温湿度数据:\r\n");
//        Serial_SendString_2("  设备地址: 0x");
//        Serial_SendHexByte_2(htm440_data.device_addr);
//        Serial_SendString_2("\r\n");
//        Serial_SendString_2("  温度: ");
//        Serial_SendFloat(htm440_data.temperature, 1);
//        Serial_SendString_2("°C\r\n");
//        Serial_SendString_2("  湿度: ");
//        Serial_SendFloat(htm440_data.humidity, 1);
//        Serial_SendString_2("%\r\n");
//        Serial_SendString_2("  上次读取时间: ");
//        Serial_SendNumber_2(task_tim1.time_1ms - htm440_data.last_read);
//        Serial_SendString_2("ms前\r\n");
    } else {
//        Serial_SendString_2("温湿度数据无效或未读取\r\n");
    }
}



// 定时器任务结构
typedef struct {
    uint16_t time_1ms;
    uint16_t time_2ms;
    uint16_t time_5ms;
    uint16_t time_10ms;
    uint16_t time_20ms;
    uint16_t time_50ms;
    uint16_t time_100ms;
    uint16_t time_200ms;
    uint16_t time_1000ms;
} Task_loop;
Task_loop task_tim = {0, 0, 0, 0, 0, 0, 0, 0,0};

// 定时器3初始化
void TIM3_Int_Init(uint16_t arr, uint16_t psc) {
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    
    TIM_TimeBaseStructure.TIM_Period = arr;
    TIM_TimeBaseStructure.TIM_Prescaler = psc;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
 
    TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 10;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    TIM_Cmd(TIM3, ENABLE);
}

// 系统时间戳
volatile uint32_t systemTick2 = 0;

// 1ms任务
void task_1ms(void) {
    systemTick2++;
    task_tim1.time_1ms++;
  
}

// 2ms任务
void task_2ms(void) {
    //LED1_Process(); // 根据需要启用
//     HTM440_Task();
}

// 5ms任务 - 使用Read_ADS1118_Direct读取传感器
void task_5ms(void) {
//    Read_ADS1118_Direct();
//    CAN_Send_All_ADC_Raw_Data();
}

// 10ms任务 - CAN通信处理
void task_10ms(void) {
////    // CAN接收处理
    if(can_rx_msg.len > 0) {
////        Serial_SendString("CAN message received: ID=0x");
////        Serial_SendHex(can_rx_msg.id);
////        Serial_SendString(", Data=");
        for(int i = 0; i < can_rx_msg.len; i++) {
////            Serial_SendHexByte(can_rx_msg.data[i]);
////            Serial_SendString(" ");
        }
////        Serial_SendString("\r\n");
        
        // 处理完成后清空消息
        can_rx_msg.len = 0;
    }
}

// 20ms任务
void task_20ms(void) {
    // 空任务
}

// 50ms任务
void task_50ms(void) {
//    Diagnose_Timing_Issues();
    // 空任务
}

// 100ms任务
void task_100ms(void) {
    // 空任务（数据已经在5ms任务中打印和发送）
    
    // 每200ms检查一次温湿度读取
        // 注意：HTM440_Task函数内部有2秒的时间间隔检查
////        HTM440_Task();
//    HTM440_ManualRead();
//           Serial_SendString_2("  湿度: ");
}

// 200ms任务
void task_200ms(void) {
    Read_ADS1118_Direct();
    CAN_Send_All_ADC_Raw_Data();
//     HTM440_Task();
    
    // 空任务
    
}

void task_1000ms(void) {

//     HTM440_Task();
    
    // 空任务
    
}


// 控制任务调度
void Control_Task(void) {
    task_tim.time_1ms++;
    task_tim.time_2ms++;
    task_tim.time_5ms++;
    task_tim.time_10ms++;
    task_tim.time_20ms++;
    task_tim.time_50ms++;
    task_tim.time_100ms++;
    task_tim.time_200ms++;
    task_tim.time_1000ms++;
    
    if(task_tim.time_1ms >= 1) {
        task_tim.time_1ms = 0;
        task_1ms();
    }
    
    if(task_tim.time_2ms >= 2) {
        task_tim.time_2ms = 0;
        task_2ms();
    }
    
    if(task_tim.time_5ms >= 5) {
        task_tim.time_5ms = 0;
        task_5ms();
    }
    
    if(task_tim.time_10ms >= 10) {
        task_tim.time_10ms = 0;
        task_10ms();
    }
    
    if(task_tim.time_20ms >= 20) {
        task_tim.time_20ms = 0;
        task_20ms();
    }
    
    if(task_tim.time_50ms >= 50) {
        task_tim.time_50ms = 0;
        task_50ms();
    }
    
    if(task_tim.time_100ms >= 100) {
        task_tim.time_100ms = 0;
        task_100ms();
    }
    
    if(task_tim.time_200ms >= 200) {
        task_tim.time_200ms = 0;
        task_200ms();
    }
    
    if(task_tim.time_1000ms >= 1000) {
        task_tim.time_1000ms = 0;
        task_1000ms();
    }
}

uint8_t timer3_flag = 0;
// 定时器3中断服务程序
////void TIM3_IRQHandler(void) {
////    if(TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) {
////        timer3_flag = 1;
////        Control_Task();
////        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
////    }
////}


// 简化TIM3中断，只更新时间
void TIM3_IRQHandler(void) {
    if(TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) {
        task_tim1.time_1ms++;  // 只更新时间
        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
    }
}


// 改进的初始化函数，添加错误检查
void Initialize_Composite_Filter_Structs(void) {
    float sampling_frequency = 1000.0f; // 根据实际定时器频率调整
    int success_count = 0;
    
    // 初始化所有滤波器
    if(init_composite_filter(&comp_filter_spi1_chip1_ain0, sampling_frequency, 10)) success_count++;
    if(init_composite_filter(&comp_filter_spi1_chip1_ain1, sampling_frequency, 10)) success_count++;
    if(init_composite_filter(&comp_filter_spi1_chip1_ain2, sampling_frequency, 10)) success_count++;
    if(init_composite_filter(&comp_filter_spi1_chip1_ain3, sampling_frequency, 10)) success_count++;
    
    if(init_composite_filter(&comp_filter_spi1_chip2_ain0, sampling_frequency, 10)) success_count++;
    if(init_composite_filter(&comp_filter_spi1_chip2_ain1, sampling_frequency, 10)) success_count++;
    if(init_composite_filter(&comp_filter_spi1_chip2_ain2, sampling_frequency, 10)) success_count++;
    if(init_composite_filter(&comp_filter_spi1_chip2_ain3, sampling_frequency, 10)) success_count++;
    
    if(init_composite_filter(&comp_filter_spi2_ain0, sampling_frequency, 10)) success_count++;
    if(init_composite_filter(&comp_filter_spi2_ain1, sampling_frequency, 10)) success_count++;
    if(init_composite_filter(&comp_filter_spi2_ain2, sampling_frequency, 10)) success_count++;
    if(init_composite_filter(&comp_filter_spi2_ain3, sampling_frequency, 10)) success_count++;
    
//    Serial_SendString("Composite Filters: ");
//    Serial_SendNumber(success_count);
//    Serial_SendString("/");
//    Serial_SendNumber(total_filters);
//    Serial_SendString(" initialized successfully\r\n");
//    
//    if(success_count != total_filters) {
//        Serial_SendString("WARNING: Some filters failed to initialize!\r\n");
//    }
}


//// 初始化复合滤波结构体
//void Initialize_Composite_Filter_Structs(void) {
//    float sampling_frequency = 1000.0f; // 假设采样频率为1kHz
//    
//    // SPI1第一个芯片
//    init_composite_filter(&comp_filter_spi1_chip1_ain0, sampling_frequency, 10);
//    init_composite_filter(&comp_filter_spi1_chip1_ain1, sampling_frequency, 10);
//    init_composite_filter(&comp_filter_spi1_chip1_ain2, sampling_frequency, 10);
//    init_composite_filter(&comp_filter_spi1_chip1_ain3, sampling_frequency, 10);
//    
//    // SPI1第二个芯片
//    init_composite_filter(&comp_filter_spi1_chip2_ain0, sampling_frequency, 10);
//    init_composite_filter(&comp_filter_spi1_chip2_ain1, sampling_frequency, 10);
//    init_composite_filter(&comp_filter_spi1_chip2_ain2, sampling_frequency, 10);
//    init_composite_filter(&comp_filter_spi1_chip2_ain3, sampling_frequency, 10);
//    
//    // SPI2芯片
//    init_composite_filter(&comp_filter_spi2_ain0, sampling_frequency, 10);
//    init_composite_filter(&comp_filter_spi2_ain1, sampling_frequency, 10);
//    init_composite_filter(&comp_filter_spi2_ain2, sampling_frequency, 10);
//    init_composite_filter(&comp_filter_spi2_ain3, sampling_frequency, 10);
//    
//    Serial_SendString_2("Composite filter structures initialized\r\n");
//}
