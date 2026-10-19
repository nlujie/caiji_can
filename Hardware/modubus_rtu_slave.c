#include "modbus_rtu_slave.h"
#include "serial.h" 
#include "serial1.h"
#include "serial2.h" 
#include <string.h>
#include <stdbool.h>

// Modbus保持寄存器
static volatile uint16_t holdingRegisters[MODBUS_REG_COUNT] = {0};

 //接收状态机
static enum {
    MB_IDLE,
    MB_RECEIVING
} rxState = MB_IDLE;

// 声明外部函数（在main.c中实现）
extern uint32_t GetTick(void);

static uint8_t rxBuffer[64];
static uint8_t rxIndex = 0;
static uint32_t lastRxTime = 0;
volatile uint8_t frameReceiving = 0;

// 定义Modbus帧超时时间（3.5字符时间 + 容差）
#define MODBUS_FRAME_TIMEOUT_MS 5

// CRC16计算
static uint16_t ModbusCRC16(uint8_t *data, uint16_t length) {
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}


// Modbus响应发送函数（使用DMA）
void Modbus_SendResponse_DMA(uint8_t *data, uint16_t length) {
    bool uart1_sent = Serial_SendBuffer_DMA(data, length);  // 串口1
    bool uart2_sent = Serial_SendBuffer_DMA_2(data, length);  // 串口2
    
    if(uart1_sent && uart2_sent) {
        // 两个串口都发送成功
    } else {
        if(!uart1_sent) {
            Serial_SendString("UART1 DMA busy\r\n");
        }
        if(!uart2_sent) {
            Serial_SendString_2("UART2 DMA busy\r\n");
        }
    }
}

// 发送响应
static void SendResponse(uint8_t *data, uint16_t length) {
    for(uint16_t i = 0; i < length; i++) {
        Serial_SendByte(data[i]);
//        Serial_SendByte_1(data[i]);
        Serial_SendByte_2(data[i]);
    }
}

// 根据功能码获取期望的帧长度
static uint16_t GetExpectedFrameLength(uint8_t *frame, uint8_t currentLength) {
    if (currentLength < 2) return 0; // 至少需要地址和功能码
    
    uint8_t functionCode = frame[1];
    
    // 根据功能码确定期望的帧长度
    switch (functionCode) {
        case 0x01: // 读线圈
        case 0x02: // 读离散输入
        case 0x03: // 读保持寄存器
        case 0x04: // 读输入寄存器
            return 8; // 地址1 + 功能码1 + 起始地址2 + 数量2 + CRC2 = 8字节
        
        case 0x05: // 写单个线圈
        case 0x06: // 写单个寄存器
            return 8; // 地址1 + 功能码1 + 寄存器地址2 + 值2 + CRC2 = 8字节
        
        case 0x0F: // 写多个线圈
        case 0x10: // 写多个寄存器
            if (currentLength >= 7) {
                uint8_t byteCount = frame[6];
                return 9 + byteCount; // 地址1 + 功能码1 + 起始地址2 + 数量2 + 字节计数1 + 数据N + CRC2
            }
            return 0;
            
        default:
            // 对于未知功能码，返回最小有效长度
            return (currentLength >= 4) ? currentLength : 0;
    }
}

// 处理Modbus请求
static void ProcessRequest(void) {
    // 调试信息
////    Serial_SendString_2("Processing Modbus frame, length: ");
  ////  Serial_SendNumber(rxIndex);
   //// Serial_SendString_2("\r\n");
    
    // 打印接收到的数据（调试用）
  ////  Serial_SendString_2("Received data: ");
    for(int i = 0; i < rxIndex; i++) {
     ////   Serial_SendHexByte(rxBuffer[i]);
    /////    Serial_SendString_2(" ");
    }
  ////  Serial_SendString_2("\r\n");
    
    // 检查最小帧长度
    if (rxIndex < 4) {
       //// Serial_SendString_2("Frame too short, discarded\r\n");
        return;
    }
    
    // CRC校验
    uint16_t crc = ModbusCRC16(rxBuffer, rxIndex - 2);
    if((rxBuffer[rxIndex - 2] != (crc & 0xFF)) || 
       (rxBuffer[rxIndex - 1] != (crc >> 8))) {
     ////   Serial_SendString_2("CRC error, frame discarded\r\n");
        return; // CRC错误丢弃
    }
    
    uint8_t addr = rxBuffer[0];
    uint8_t function = rxBuffer[1];
    
    // 检查地址匹配
    if(addr != MODBUS_ADDRESS) {
      ////  Serial_SendString_2("Address mismatch, expected: ");
       //// Serial_SendNumber(MODBUS_ADDRESS);
       //// Serial_SendString_2(", received: ");
      ////  Serial_SendNumber(addr);
      ////  Serial_SendString_2("\r\n");
        return;
    }
    
  ////  Serial_SendString_2("Valid Modbus frame - Address: ");
  ////  Serial_SendNumber(addr);
  ////  Serial_SendString_2(", Function: 0x");
  ////  Serial_SendHexByte(function);
   //// Serial_SendString_2("\r\n");
    
    switch(function) {
        case 0x03: { // 读保持寄存器
            uint16_t startReg = (rxBuffer[2] << 8) | rxBuffer[3];
            uint16_t regCount = (rxBuffer[4] << 8) | rxBuffer[5];
            
       ////     Serial_SendString_2("Read Holding Registers - Start: ");
////            Serial_SendNumber(startReg);
////            Serial_SendString_2(", Count: ");
////            Serial_SendNumber(regCount);
////            Serial_SendString_2("\r\n");
            
            if(startReg >= MODBUS_REG_COUNT || 
               (startReg + regCount) > MODBUS_REG_COUNT) {
                // 非法地址响应
                uint8_t exception[] = {
                    addr, 
                    function | 0x80, 
                    0x02,  // 非法数据地址
                    0, 0   // CRC占位
                };
                uint16_t crc = ModbusCRC16(exception, 3);
                exception[3] = crc & 0xFF;
                exception[4] = crc >> 8;
                Modbus_SendResponse_DMA(exception, 5);
              ////  Serial_SendString_2("Illegal data address error sent\r\n");
                return;
            }
            
            // 正常响应
            uint8_t frame[5 + regCount * 2];
            frame[0] = addr;
            frame[1] = function;
            frame[2] = regCount * 2;
            
            for(uint16_t i = 0; i < regCount; i++) {
                frame[3 + i * 2] = holdingRegisters[startReg + i] >> 8;
                frame[4 + i * 2] = holdingRegisters[startReg + i] & 0xFF;
            }
            
            crc = ModbusCRC16(frame, 3 + regCount * 2);
            frame[3 + regCount * 2] = crc & 0xFF;
            frame[4 + regCount * 2] = crc >> 8;
            
            Modbus_SendResponse_DMA(frame, 5 + regCount * 2);
         ////   Serial_SendString_2("Read response sent\r\n");
            break;
        }
        
        case 0x06: { // 写单个寄存器
            uint16_t regAddr = (rxBuffer[2] << 8) | rxBuffer[3];
            uint16_t regValue = (rxBuffer[4] << 8) | rxBuffer[5];
            
        ////    Serial_SendString_2("Write Single Register - Addr: ");
          ////  Serial_SendNumber(regAddr);
          ////  Serial_SendString_2(", Value: ");
          ////  Serial_SendNumber(regValue);
           //// Serial_SendString_2("\r\n");
            
            if(regAddr >= MODBUS_REG_COUNT) {
                // 非法地址响应
                uint8_t exception[] = {
                    addr, 
                    function | 0x80, 
                    0x02,  // 非法数据地址
                    0, 0   // CRC占位
                };
                uint16_t crc = ModbusCRC16(exception, 3);
                exception[3] = crc & 0xFF;
                exception[4] = crc >> 8;
                Modbus_SendResponse_DMA(exception, 5);
             ////   Serial_SendString_2("Illegal data address error sent\r\n");
                return;
            }
            
            // 写入寄存器
            holdingRegisters[regAddr] = regValue;
            
            // 回显请求作为响应（Modbus写单个寄存器协议）
            Modbus_SendResponse_DMA(rxBuffer, rxIndex);
           //// Serial_SendString_2("Write response sent\r\n");
            break;
        }
            
        default:
            // 不支持的功能码响应
            uint8_t exception[] = {
                addr, 
                function | 0x80, 
                0x01,  // 非法功能码
                0, 0   // CRC占位
            };
            crc = ModbusCRC16(exception, 3);
            exception[3] = crc & 0xFF;
            exception[4] = crc >> 8;
            Modbus_SendResponse_DMA(exception, 5);
         ////   Serial_SendString_2("Illegal function error sent\r\n");
            break;
    }
}

// 初始化Modbus
void ModbusRTU_Init(void) {
    // 初始化寄存器默认值
    for(int i = 0; i < MODBUS_REG_COUNT; i++) {
        holdingRegisters[i] = 0;
    }
    rxState = MB_IDLE;
    rxIndex = 0;
    lastRxTime = 0;
    frameReceiving = 0;
}

// 获取寄存器值
uint16_t ModbusRTU_GetRegister(uint8_t reg) {
    if(reg >= MODBUS_REG_COUNT) return 0;
    return holdingRegisters[reg];
}

// 设置寄存器值
void ModbusRTU_SetRegister(uint8_t reg, uint16_t value) {
    if(reg < MODBUS_REG_COUNT) {
        holdingRegisters[reg] = value;
    }
}

// 修复的接收数据处理函数
void ModbusRTU_ReceiveHandler(uint8_t data) {
    uint32_t currentTime = GetTick();
    
    // 检查帧间隔超时（新帧开始）
    if (rxIndex > 0 && (currentTime - lastRxTime) > MODBUS_FRAME_TIMEOUT_MS) {
        // 超时，重置接收状态（丢弃不完整帧）
////        Serial_SendString_2("Frame timeout, resetting buffer. Index: ");
////        Serial_SendNumber(rxIndex);
////        Serial_SendString_2("\r\n");
        rxIndex = 0;
    }
    
    // 存储接收到的数据
    if (rxIndex < sizeof(rxBuffer)) {
        rxBuffer[rxIndex++] = data;
        lastRxTime = currentTime;
        frameReceiving = 1;
        
        // 调试信息
     ////   Serial_SendString_2("Received byte: 0x");
     ////   Serial_SendHexByte(data);
////        Serial_SendString_2(", Index: ");
////        Serial_SendNumber(rxIndex);
////        Serial_SendString_2("\r\n");
    } else {
        // 缓冲区溢出
//        Serial_SendString_2("Receive buffer overflow!\r\n");
        rxIndex = 0;
        frameReceiving = 0;
    }
}

// 修复的轮询函数
void ModbusRTU_Poll(void) {
    uint32_t currentTime = GetTick();
    
    // 没有数据时直接返回
    if (rxIndex == 0) return;
    
    // 检查帧超时
    if ((currentTime - lastRxTime) > MODBUS_FRAME_TIMEOUT_MS) {
     ////   Serial_SendString_2("Frame complete detected. Length: ");
////        Serial_SendNumber(rxIndex);
////        Serial_SendString_2("\r\n");
        
        // 获取期望的帧长度
        uint16_t expectedLength = GetExpectedFrameLength(rxBuffer, rxIndex);
        
        if (expectedLength > 0 && rxIndex >= expectedLength) {
            // 帧长度符合预期，处理请求
            ProcessRequest();
        } else {
            Serial_SendString_2("Invalid frame length. Expected: ");
            Serial_SendNumber(expectedLength);
            Serial_SendString_2(", Received: ");
            Serial_SendNumber(rxIndex);
            Serial_SendString_2("\r\n");
        }
        
        // 重置接收状态
        rxIndex = 0;
        frameReceiving = 0;
    }
}

