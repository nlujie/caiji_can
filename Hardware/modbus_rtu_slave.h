#ifndef __MODBUS_RTU_SLAVE_H
#define __MODBUS_RTU_SLAVE_H

#include "stm32f10x.h"
#include <stdint.h>
// Modbus配置
#define MODBUS_ADDRESS     0x01    // 从站地址
#define MODBUS_REG_COUNT   16      // 保持寄存器数量
#define MODBUS_RX_BUF_SIZE 64      // 接收缓冲区大小


// ... 其他寄存器定义

// 初始化函数
void ModbusRTU_Init(void);

//void ModbusRTU_ReceiveHandler(uint8_t data);

void ModbusRTU_Process(void);
// 寄存器访问接口
void ModbusRTU_SetRegister(uint8_t reg, uint16_t value);
uint16_t ModbusRTU_GetRegister(uint8_t reg);

// 处理接收到的数据
void ModbusRTU_ReceiveHandler(uint8_t data);


void ModbusRTU_Poll(void);


#endif
