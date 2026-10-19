#ifndef __MODBUS_RTU_MASTER_H
#define __MODBUS_RTU_MASTER_H

#include "stm32f10x.h"
#include <stdint.h>

// Modbus RTU主站配置
#define MODBUS_SLAVE_ADDRESS      0x01    // HTM440默认地址
#define MODBUS_BAUDRATE           9600    // 波特率
#define MODBUS_PARITY             0       // 0:无校验, 1:奇校验, 2:偶校验
#define MODBUS_STOPBITS           1       // 停止位

// 寄存器地址（根据HTM440手册）
#define TEMP_REG_ADDR             0x0002  // 温度寄存器地址
#define HUMIDITY_REG_ADDR         0x0003  // 湿度寄存器地址
#define DEVICE_ADDR_REG           0x0000  // 设备地址寄存器

// Modbus功能码
#define MODBUS_READ_HOLDING_REG   0x03    // 读保持寄存器

// 温湿度数据结构
// 温湿度数据结构
typedef struct {
    float temperature;     // 温度值（实际值）
    float humidity;        // 湿度值（实际值）
    int16_t raw_temperature; // 原始温度值（实际值×10）
    int16_t raw_humidity;    // 原始湿度值（实际值×10）
    uint8_t device_addr;   // 设备地址
    uint8_t valid;         // 数据有效标志
    uint32_t last_read;    // 上次读取时间
    uint8_t read_status;   // 读取状态
      
} HTM440_Data_t;

// 错误码定义
typedef enum {
    MODBUS_MASTER_OK = 0,
    MODBUS_MASTER_TIMEOUT,
    MODBUS_MASTER_CRC_ERROR,
    MODBUS_MASTER_INVALID_RESPONSE,
    MODBUS_MASTER_INVALID_ADDRESS,
    MODBUS_MASTER_INVALID_DATA,
     MODBUS_MASTER_BUSY,   
    MODBUS_MASTER_WAITING
} ModbusMaster_Status;

// 函数声明
void ModbusMaster_Init(void);
ModbusMaster_Status HTM440_Read_Temperature_Humidity(uint8_t slave_addr, HTM440_Data_t *data);
ModbusMaster_Status HTM440_Read_Temperature(uint8_t slave_addr, HTM440_Data_t *data);
ModbusMaster_Status HTM440_Read_Humidity(uint8_t slave_addr, HTM440_Data_t *data);
ModbusMaster_Status HTM440_Read_Device_Address(uint8_t *addr);
uint16_t Modbus_CRC16(uint8_t *data, uint16_t length);
uint8_t ModbusMaster_SendRequest(uint8_t slave_addr, uint8_t function, 
                                uint16_t start_addr, uint16_t reg_count);
uint8_t ModbusMaster_ReceiveResponse(uint8_t *data, uint16_t *length, 
                                    uint32_t timeout_ms);

//uint8_t ModbusMaster_ReceiveResponse(uint8_t *data, uint16_t *length 
//                                   );


void ModbusMaster_ResetBuffer(void);

// 外部变量声明（这些变量在serial.c中定义）
extern volatile uint8_t modbus_rx_complete;
extern uint8_t rx_buffer_3[];
extern volatile uint16_t rx_index_3;

#endif /* __MODBUS_RTU_MASTER_H */
