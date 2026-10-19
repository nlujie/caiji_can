#include "stm32f10x.h"
#include <stdio.h>
#include "Delay.h"
#include "modbus_rtu_slave.h"
#include "func.h"
#include "serial.h"

// 接收缓冲区（中断接收）
#define RX_BUFFER_SIZE_3 128
uint8_t rx_buffer_3[RX_BUFFER_SIZE_3];
volatile uint16_t rx_index_3 = 0;
volatile uint8_t rx_complete_3 = 0;

// 发送缓冲区（DMA发送）
#define TX_BUFFER_SIZE_3 128
uint8_t tx_buffer_3[TX_BUFFER_SIZE_3];
volatile uint8_t tx_busy_3 = 0; // DMA发送忙标志

// 添加 Modbus 主站接收完成标志（在serial.c中定义）
volatile uint8_t modbus_rx_complete = 0;


// DMA发送配置
void DMA_TX_Configuration_USART3(void)
{
    DMA_InitTypeDef DMA_InitStructure;
    
    // 开启DMA时钟
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    
    // 配置DMA1通道2 (USART3_TX)
    DMA_DeInit(DMA1_Channel2);
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(USART3->DR);
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)tx_buffer_3;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST; // 内存到外设
    DMA_InitStructure.DMA_BufferSize = 0; // 初始为0
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal; // 正常模式（非循环）
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel2, &DMA_InitStructure);
    
    // 配置DMA发送完成中断
    DMA_ITConfig(DMA1_Channel2, DMA_IT_TC, ENABLE);
    
    // DMA发送中断配置
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 11;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_Init(&NVIC_InitStructure);
}

void Serial_Init_2()
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // TX引脚配置
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    // RX引脚配置
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate = 9600;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_Init(USART3, &USART_InitStructure);
    
    // 配置接收中断（RXNE中断）
    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
    
    // 配置空闲中断（用于检测帧结束）
    USART_ITConfig(USART3, USART_IT_IDLE, ENABLE);
    
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    
    // USART3中断配置
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_Init(&NVIC_InitStructure);
    
    // 初始化DMA发送（不再使用DMA接收）
    DMA_TX_Configuration_USART3();
    
    USART_Cmd(USART3, ENABLE);
}

// USART3中断处理函数 - 中断接收
void USART3_IRQHandler(void)
{
    // 接收中断处理
    if(USART_GetITStatus(USART3, USART_IT_RXNE) == SET)
    {
        // 读取接收到的数据
        uint8_t data = USART_ReceiveData(USART3);
        
      
        
        // 存储到接收缓冲区
        if(rx_index_3 < RX_BUFFER_SIZE_3) {
            rx_buffer_3[rx_index_3++] = data;
        } else {
            // 缓冲区溢出，从头开始
            rx_index_3 = 0;
            rx_buffer_3[rx_index_3++] = data;
        }
        
//        // 可以在这里直接处理每个字节，或者等待空闲中断
//         ModbusRTU_ReceiveHandler(data);
    }
    
    // 空闲中断处理（帧结束检测）
    if(USART_GetITStatus(USART3, USART_IT_IDLE) == SET)
    {
        // 清除空闲中断标志（通过读SR和DR寄存器）
        USART_ReceiveData(USART3); // 读取DR寄存器清除标志
        
        // 帧接收完成，设置标志
        if(rx_index_3 > 0) {
//            rx_complete_3 = 1;
            
            modbus_rx_complete = 1;
            // 调试信息
           //// Serial_SendString_2("Frame received, length: ");
           //// Serial_SendNum_2(rx_index_3, 3);
           //// Serial_SendString_2("\r\n");
        }
    }
}

// DMA发送完成中断
void DMA1_Channel2_IRQHandler(void)
{
    if(DMA_GetITStatus(DMA1_IT_TC2) == SET)
    {
        // 清除中断标志
        DMA_ClearITPendingBit(DMA1_IT_TC2);
        
        // 禁用DMA通道
        DMA_Cmd(DMA1_Channel2, DISABLE);
        
        // 禁用USART3的DMA发送
        USART_DMACmd(USART3, USART_DMAReq_Tx, DISABLE);
        
        // 清除发送忙标志
        tx_busy_3 = 0;
        
        // 可以在这里添加发送完成回调
     ////   Serial_SendString_1("DMA send complete\r\n");
    }
}

// DMA发送函数（非阻塞）
uint8_t Serial_SendBuffer_DMA_2(uint8_t *buffer, uint16_t length)
{
    if(tx_busy_3) {
        return 0; // DMA正忙，发送失败
    }
    
    if(length > TX_BUFFER_SIZE_3) {
        length = TX_BUFFER_SIZE_3; // 截断
    }
    
    // 复制数据到发送缓冲区
    for(uint16_t i = 0; i < length; i++) {
        tx_buffer_3[i] = buffer[i];
    }
    
    // 配置DMA传输长度
    DMA_SetCurrDataCounter(DMA1_Channel2, length);
    
    // 设置发送忙标志
    tx_busy_3 = 1;
    
    // 使能DMA通道
    DMA_Cmd(DMA1_Channel2, ENABLE);
    
    // 使能USART3的DMA发送
    USART_DMACmd(USART3, USART_DMAReq_Tx, ENABLE);
    
    return 1; // 发送启动成功
}

// 兼容原有的查询发送函数（可选保留）
void Serial_SendByte_2(uint8_t Byte)
{
    // 如果DMA不忙，使用DMA发送单个字节
    uint8_t buffer[1] = {Byte};
    if(!Serial_SendBuffer_DMA_2(buffer, 1)) {
        // DMA忙，使用查询发送作为后备
        while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
        USART_SendData(USART3, Byte);
    }
}

// 批量发送函数（使用DMA）
void Serial_SendArray_2(uint8_t *array, uint16_t length)
{
    if(Serial_SendBuffer_DMA_2(array, length)) {
        // 等待DMA发送完成（可选，如果是非阻塞可以跳过）
        while(tx_busy_3) {
            // 可以在这里执行其他任务
            Delay_ms(1);
        }
    } else {
        // DMA忙，使用查询发送作为后备
        for(uint16_t i = 0; i < length; i++) {
            while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
            USART_SendData(USART3, array[i]);
        }
    }
}

// 检查DMA发送状态
uint8_t Serial_DMA_Busy_2(void)
{
    return tx_busy_3;
}

// 其他辅助函数保持不变
void Serial_SendHexByte_2(uint8_t byte) {
    uint8_t nibbles[2];
    nibbles[0] = (byte >> 4) & 0x0F;
    nibbles[1] = byte & 0x0F;
    
    for(int i = 0; i < 2; i++) {
        if(nibbles[i] < 10) {
            Serial_SendByte_2('0' + nibbles[i]);
        } else {
            Serial_SendByte_2('A' + (nibbles[i] - 10));
        }
    }
}

void Serial_SendHex_2(uint16_t num)
{
    char hex_chars[] = "0123456789ABCDEF";
    Serial_SendByte_2(hex_chars[(num >> 12) & 0x0F]);
    Serial_SendByte_2(hex_chars[(num >> 8) & 0x0F]);
    Serial_SendByte_2(hex_chars[(num >> 4) & 0x0F]);
    Serial_SendByte_2(hex_chars[num & 0x0F]);
}


void Serial_SendString_2(const char *string) {
    for(uint16_t i = 0; string[i] != '\0'; i++) {
        Serial_SendByte_2(string[i]);
    }
}

void Serial_SendNumber_2(int32_t number)
{
    char buffer[12];
    int i = 0;
    int is_negative = 0;
    
    if (number < 0) {
        is_negative = 1;
        number = -number;
    }
    
    if (number == 0) {
        Serial_SendByte_2('0');
        return;
    }
    
    while (number > 0) {
        buffer[i++] = '0' + (number % 10);
        number /= 10;
    }
    
    if (is_negative) {
        buffer[i++] = '-';
    }
    
    for (int j = i - 1; j >= 0; j--) {
        Serial_SendByte_2(buffer[j]);
    }
}




/*
// Modbus响应发送函数（使用DMA）
void Modbus_SendResponse_DMA(uint8_t *data, uint16_t length) {
    if(Serial_SendBuffer_DMA_2(data, length)) {
       // Serial_SendString_2("Modbus response sent via DMA\r\n");
    } else {
        Serial_SendString_2("DMA busy, Modbus response delayed\r\n");
        // 可以在这里实现发送队列
    }
}
*/

