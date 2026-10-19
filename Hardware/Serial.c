/*
#include "stm32f10x.h"
#include <stdio.h>
#include "Delay.h"
#include "modbus_rtu_slave.h"
#include "func.h"

// DMA接收缓冲区
#define RX_BUFFER_SIZE_1 128
uint8_t rx_buffer_1[RX_BUFFER_SIZE_1];

// DMA通道配置 - USART1_RX使用DMA1通道5
void DMA_Configuration_USART1(void)
{
    DMA_InitTypeDef DMA_InitStructure;
    
    // 开启DMA时钟
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    
    // 配置DMA1通道5 (USART1_RX)
    DMA_DeInit(DMA1_Channel5);
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(USART1->DR);
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)rx_buffer_1;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_BufferSize = RX_BUFFER_SIZE_1;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular; // 循环模式
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel5, &DMA_InitStructure);
    
    // 开启DMA通道
    DMA_Cmd(DMA1_Channel5, ENABLE);
    
    // 配置USART1 DMA接收
    USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE);
}

void Serial_Init()
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate = 9600;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_Init(USART1, &USART_InitStructure);
    
    // 配置空闲中断用于检测帧结束
    USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);
    
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_Init(&NVIC_InitStructure);
    
    // 初始化DMA
    DMA_Configuration_USART1();
    
    USART_Cmd(USART1, ENABLE);
}

// USART1中断处理函数 - 现在只处理空闲中断
void USART1_IRQHandler(void)
{
    if(USART_GetITStatus(USART1, USART_IT_IDLE) == SET)
    {
        // 清除空闲中断标志
        USART_ReceiveData(USART1); // 读取DR寄存器清除标志
        
        // 计算接收到的数据长度
        uint16_t data_length = RX_BUFFER_SIZE_1 - DMA_GetCurrDataCounter(DMA1_Channel5);
        
        // 处理接收到的数据
        for(uint16_t i = 0; i < data_length; i++)
        {
            ModbusRTU_ReceiveHandler(rx_buffer_1[i]);
        }
        
        // 重置DMA接收
        DMA_Cmd(DMA1_Channel5, DISABLE);
        DMA_SetCurrDataCounter(DMA1_Channel5, RX_BUFFER_SIZE_1);
        DMA_Cmd(DMA1_Channel5, ENABLE);
    }
}

// 发送函数保持不变...
void Serial_SendByte(uint8_t Byte)
{
    // 等待发送寄存器空
    while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    
    // 发送数据
    USART_SendData(USART1, Byte);
    
    // 关键：等待发送完全完成（包括停止位）
    while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
    
    // 清除TC标志
    USART_ClearFlag(USART1, USART_FLAG_TC);
    
    // 额外延时确保485模块稳定
    Delay_us(100);  // 100μs延时
}

void Serial_SendArray(uint8_t *array, uint16_t length)
{
    uint16_t i;
    for(i = 0; i < length; i++)
    {
        Serial_SendByte(array[i]);
    }
}

void Serial_SendString(char *string)
{
    uint16_t i;
    for(i = 0; string[i] != '\0'; i++)
    {
        Serial_SendByte(string[i]);
    }
}

uint32_t Serial_pow(uint32_t x, uint32_t y)
{
    uint32_t result = 1;
    while(y--)
    {
        result *= x;
    }
    return result;
}

void Serial_SendNum(uint32_t Num, uint16_t length)
{
    uint32_t i;
    for(i = 0; i < length; i++)
    {
        Serial_SendByte(Num / Serial_pow(10, length - i - 1) % 10 + '0');
    }
}

int fputc(int ch, FILE *f)
{
    Serial_SendByte(ch);
    return ch;
}

void USART1_SendBuffer(uint8_t *buffer, uint16_t len)
{
    for(uint16_t i = 0; i < len; i++)
    {
        USART_SendData(USART1, buffer[i]);
        while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    }
}
*/
#include "stm32f10x.h"
#include <stdio.h>
#include "Delay.h"
#include "modbus_rtu_slave.h"
#include "func.h"

// 接收缓冲区（中断接收）
#define RX_BUFFER_SIZE_1 128
uint8_t rx_buffer_1[RX_BUFFER_SIZE_1];
volatile uint16_t rx_index_1 = 0;
volatile uint8_t rx_complete_1 = 0;

// 发送缓冲区（DMA发送）
#define TX_BUFFER_SIZE_1 128
uint8_t tx_buffer_1[TX_BUFFER_SIZE_1];
volatile uint8_t tx_busy_1 = 0; // DMA发送忙标志

//// 添加 Modbus 主站接收完成标志（在serial.c中定义）
//volatile uint8_t modbus_rx_complete = 0;

// DMA发送配置 - USART1_TX使用DMA1通道4
void DMA_TX_Configuration_USART1(void)
{
    DMA_InitTypeDef DMA_InitStructure;
    
    // 开启DMA时钟
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    
    // 配置DMA1通道4 (USART1_TX)
    DMA_DeInit(DMA1_Channel4);
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(USART1->DR);
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)tx_buffer_1;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST; // 内存到外设
    DMA_InitStructure.DMA_BufferSize = 0; // 初始为0
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal; // 正常模式（非循环）
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel4, &DMA_InitStructure);
    
    // 配置DMA发送完成中断
    DMA_ITConfig(DMA1_Channel4, DMA_IT_TC, ENABLE);
    
    // DMA发送中断配置
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel4_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_Init(&NVIC_InitStructure);
}

void Serial_Init()
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // TX引脚配置
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // RX引脚配置
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate = 9600;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_Init(USART1, &USART_InitStructure);
    
    // 配置接收中断（RXNE中断）- 中断接收
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    
    // 配置空闲中断（用于检测帧结束）
    USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);
    
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    
    // USART1中断配置
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_Init(&NVIC_InitStructure);
    
    // 初始化DMA发送（不再使用DMA接收）
    DMA_TX_Configuration_USART1();
    
    USART_Cmd(USART1, ENABLE);
}

// USART1中断处理函数 - 中断接收
void USART1_IRQHandler(void)
{
    // 接收中断处理
    if(USART_GetITStatus(USART1, USART_IT_RXNE) == SET)
    {
        // 读取接收到的数据
        uint8_t data = USART_ReceiveData(USART1);
        
        // 存储到接收缓冲区
        if(rx_index_1 < RX_BUFFER_SIZE_1) {
            rx_buffer_1[rx_index_1++] = data;
        } else {
            // 缓冲区溢出，从头开始
            rx_index_1 = 0;
            rx_buffer_1[rx_index_1++] = data;
        }
        
        // 可以在这里直接处理每个字节
     //   ModbusRTU_ReceiveHandler(data);
    }
    
    // 空闲中断处理（帧结束检测）
    if(USART_GetITStatus(USART1, USART_IT_IDLE) == SET)
    {
        // 清除空闲中断标志（通过读SR和DR寄存器）
        USART_ReceiveData(USART1); // 读取DR寄存器清除标志
        
        // 帧接收完成，设置标志
        if(rx_index_1 > 0) {
            rx_complete_1 = 1;
            // 设置Modbus接收完成标志
//            modbus_rx_complete = 1;
            // 调试信息
          //  Serial_SendString_1("USART1 Frame received, length: ");
          //  Serial_SendNum_1(rx_index_1, 3);
          //  Serial_SendString_1("\r\n");
        }
    }
}

// DMA发送完成中断 - USART1
void DMA1_Channel4_IRQHandler(void)
{
    if(DMA_GetITStatus(DMA1_IT_TC4) == SET)
    {
        // 清除中断标志
        DMA_ClearITPendingBit(DMA1_IT_TC4);
        
        // 禁用DMA通道
        DMA_Cmd(DMA1_Channel4, DISABLE);
        
        // 禁用USART1的DMA发送
        USART_DMACmd(USART1, USART_DMAReq_Tx, DISABLE);
        
        // 清除发送忙标志
        tx_busy_1 = 0;
        
        // 发送完成回调
       // Serial_SendString_1("USART1 DMA send complete\r\n");
    }
}

// DMA发送函数（非阻塞）- USART1
uint8_t Serial_SendBuffer_DMA(uint8_t *buffer, uint16_t length)
{
    if(tx_busy_1) {
        return 0; // DMA正忙，发送失败
    }
    
    if(length > TX_BUFFER_SIZE_1) {
        length = TX_BUFFER_SIZE_1; // 截断
    }
    
    // 复制数据到发送缓冲区
    for(uint16_t i = 0; i < length; i++) {
        tx_buffer_1[i] = buffer[i];
    }
    
    // 配置DMA传输长度
    DMA_SetCurrDataCounter(DMA1_Channel4, length);
    
    // 设置发送忙标志
    tx_busy_1 = 1;
    
    // 使能DMA通道
    DMA_Cmd(DMA1_Channel4, ENABLE);
    
    // 使能USART1的DMA发送
    USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);
    
    return 1; // 发送启动成功
}

// 兼容原有的查询发送函数（可选保留）
void Serial_SendByte(uint8_t Byte)
{
    // 如果DMA不忙，使用DMA发送单个字节
    uint8_t buffer[1] = {Byte};
    if(!Serial_SendBuffer_DMA(buffer, 1)) {
        // DMA忙，使用查询发送作为后备
        while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
        USART_SendData(USART1, Byte);
    }
}

// 批量发送函数（使用DMA）
void Serial_SendArray(uint8_t *array, uint16_t length)
{
    if(Serial_SendBuffer_DMA(array, length)) {
        // 等待DMA发送完成（可选，如果是非阻塞可以跳过）
        while(tx_busy_1) {
            // 可以在这里执行其他任务
            Delay_ms(1);
        }
    } else {
        // DMA忙，使用查询发送作为后备
        for(uint16_t i = 0; i < length; i++) {
            while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
            USART_SendData(USART1, array[i]);
        }
    }
}

// 检查DMA发送状态
uint8_t Serial_DMA_Busy(void)
{
    return tx_busy_1;
}

// 其他辅助函数
void Serial_SendHexByte(uint8_t byte)
{
    uint8_t nibbles[2];
    nibbles[0] = (byte >> 4) & 0x0F;
    nibbles[1] = byte & 0x0F;
    
    for(int i = 0; i < 2; i++) {
        if(nibbles[i] < 10) {
            Serial_SendByte('0' + nibbles[i]);
        } else {
            Serial_SendByte('A' + (nibbles[i] - 10));
        }
    }
}

void Serial_SendHex(uint16_t num)
{
    char hex_chars[] = "0123456789ABCDEF";
    Serial_SendByte(hex_chars[(num >> 12) & 0x0F]);
    Serial_SendByte(hex_chars[(num >> 8) & 0x0F]);
    Serial_SendByte(hex_chars[(num >> 4) & 0x0F]);
    Serial_SendByte(hex_chars[num & 0x0F]);
}

void Serial_SendNumber(int32_t number)
{
    char buffer[12];
    int i = 0;
    int is_negative = 0;
    
    if (number < 0) {
        is_negative = 1;
        number = -number;
    }
    
    if (number == 0) {
        Serial_SendByte('0');
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
        Serial_SendByte(buffer[j]);
    }
}

void Serial_SendString(char *string)
{
    for(uint16_t i = 0; string[i] != '\0'; i++) {
        Serial_SendByte(string[i]);
    }
}

uint32_t Serial_pow(uint32_t x, uint32_t y)
{
    uint32_t result = 1;
    while(y--) {
        result *= x;
    }
    return result;
}

void Serial_SendNum(uint32_t Num, uint16_t length)
{
    for(uint32_t i = 0; i < length; i++) {
        Serial_SendByte(Num / Serial_pow(10, length - i - 1) % 10 + '0');
    }
}

int fputc(int ch, FILE *f)
{
    Serial_SendByte(ch);
    return ch;
}

