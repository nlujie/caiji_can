// #include "stm32f10x.h"
//#include <stdio.h>
//#include "Delay.h"
//#include "modbus_rtu_slave.h"
//#include "func.h"

//// 接收缓冲区（中断接收）
//#define RX_BUFFER_SIZE_2 128
//uint8_t rx_buffer_2[RX_BUFFER_SIZE_2];
//volatile uint16_t rx_index_2 = 0;
//volatile uint8_t rx_complete_2 = 0;

//// 发送缓冲区（DMA发送）
//#define TX_BUFFER_SIZE_2 128
//uint8_t tx_buffer_2[TX_BUFFER_SIZE_2];
//volatile uint8_t tx_busy_2 = 0; // DMA发送忙标志

//// DMA发送配置 - USART2_TX使用DMA1通道7
//void DMA_TX_Configuration_USART2(void)
//{
//    DMA_InitTypeDef DMA_InitStructure;
//    
//    // 开启DMA时钟
//    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
//    
//    // 配置DMA1通道7 (USART2_TX)
//    DMA_DeInit(DMA1_Channel7);
//    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(USART2->DR);
//    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)tx_buffer_2;
//    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST; // 内存到外设
//    DMA_InitStructure.DMA_BufferSize = 0; // 初始为0
//    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
//    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
//    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
//    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
//    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal; // 正常模式（非循环）
//    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
//    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
//    DMA_Init(DMA1_Channel7, &DMA_InitStructure);
//    
//    // 配置DMA发送完成中断
//    DMA_ITConfig(DMA1_Channel7, DMA_IT_TC, ENABLE);
//    
//    // DMA发送中断配置
//    NVIC_InitTypeDef NVIC_InitStructure;
//    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel7_IRQn;
//    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
//    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
//    NVIC_Init(&NVIC_InitStructure);
//}

//void Serial_Init_1()
//{
//    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
//    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
//    
//    GPIO_InitTypeDef GPIO_InitStructure;
//    
//    // TX引脚配置
//    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
//    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
//    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//    GPIO_Init(GPIOA, &GPIO_InitStructure);
//    
//    // RX引脚配置
//    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
//    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
//    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//    GPIO_Init(GPIOA, &GPIO_InitStructure);
//    
//    USART_InitTypeDef USART_InitStructure;
//    USART_InitStructure.USART_BaudRate = 9600;
//    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
//    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
//    USART_InitStructure.USART_Parity = USART_Parity_No;
//    USART_InitStructure.USART_StopBits = USART_StopBits_1;
//    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
//    USART_Init(USART2, &USART_InitStructure);
//    
//    // 配置接收中断（RXNE中断）- 中断接收
//    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
//    
//    // 配置空闲中断（用于检测帧结束）
//    USART_ITConfig(USART2, USART_IT_IDLE, ENABLE);
//    
//    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
//    
//    // USART2中断配置
//    NVIC_InitTypeDef NVIC_InitStructure;
//    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
//    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
//    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
//    NVIC_Init(&NVIC_InitStructure);
//    
//    // 初始化DMA发送（不再使用DMA接收）
//    DMA_TX_Configuration_USART2();
//    
//    USART_Cmd(USART2, ENABLE);
//}

//// USART2中断处理函数 - 中断接收
//void USART2_IRQHandler(void)
//{
//    // 接收中断处理
//    if(USART_GetITStatus(USART2, USART_IT_RXNE) == SET)
//    {
//        // 读取接收到的数据
//        uint8_t data = USART_ReceiveData(USART2);
//        
//        // 存储到接收缓冲区
//        if(rx_index_2 < RX_BUFFER_SIZE_2) {
//            rx_buffer_2[rx_index_2++] = data;
//        } else {
//            // 缓冲区溢出，从头开始
//            rx_index_2 = 0;
//            rx_buffer_2[rx_index_2++] = data;
//        }
//        
//        // 可以在这里直接处理每个字节
//        // ModbusRTU_ReceiveHandler(data); // 如果需要可以取消注释
//    }
//    
//    // 空闲中断处理（帧结束检测）
//    if(USART_GetITStatus(USART2, USART_IT_IDLE) == SET)
//    {
//        // 清除空闲中断标志（通过读SR和DR寄存器）
//        USART_ReceiveData(USART2); // 读取DR寄存器清除标志
//        
//        // 帧接收完成，设置标志
//        if(rx_index_2 > 0) {
//            rx_complete_2 = 1;
//            
//            // 调试信息
//            Serial_SendString("USART2 Frame received, length: ");
//            Serial_SendNum(rx_index_2, 3);
//            Serial_SendString("\r\n");
//        }
//    }
//}

//// DMA发送完成中断 - USART2
//void DMA1_Channel7_IRQHandler(void)
//{
//    if(DMA_GetITStatus(DMA1_IT_TC7) == SET)
//    {
//        // 清除中断标志
//        DMA_ClearITPendingBit(DMA1_IT_TC7);
//        
//        // 禁用DMA通道
//        DMA_Cmd(DMA1_Channel7, DISABLE);
//        
//        // 禁用USART2的DMA发送
//        USART_DMACmd(USART2, USART_DMAReq_Tx, DISABLE);
//        
//        // 清除发送忙标志
//        tx_busy_2 = 0;
//        
//        // 发送完成回调
//        Serial_SendString("USART2 DMA send complete\r\n");
//    }
//}

//// DMA发送函数（非阻塞）- USART2
//uint8_t Serial_SendBuffer_DMA_1(uint8_t *buffer, uint16_t length)
//{
//    if(tx_busy_2) {
//        return 0; // DMA正忙，发送失败
//    }
//    
//    if(length > TX_BUFFER_SIZE_2) {
//        length = TX_BUFFER_SIZE_2; // 截断
//    }
//    
//    // 复制数据到发送缓冲区
//    for(uint16_t i = 0; i < length; i++) {
//        tx_buffer_2[i] = buffer[i];
//    }
//    
//    // 配置DMA传输长度
//    DMA_SetCurrDataCounter(DMA1_Channel7, length);
//    
//    // 设置发送忙标志
//    tx_busy_2 = 1;
//    
//    // 使能DMA通道
//    DMA_Cmd(DMA1_Channel7, ENABLE);
//    
//    // 使能USART2的DMA发送
//    USART_DMACmd(USART2, USART_DMAReq_Tx, ENABLE);
//    
//    return 1; // 发送启动成功
//}

//// 兼容原有的查询发送函数（可选保留）
//void Serial_SendByte_1(uint8_t Byte)
//{
//    // 如果DMA不忙，使用DMA发送单个字节
//    uint8_t buffer[1] = {Byte};
//    if(!Serial_SendBuffer_DMA_1(buffer, 1)) {
//        // DMA忙，使用查询发送作为后备
//        while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
//        USART_SendData(USART2, Byte);
//    }
//}

//// 批量发送函数（使用DMA）
//void Serial_SendArray_1(uint8_t *array, uint16_t length)
//{
//    if(Serial_SendBuffer_DMA_1(array, length)) {
//        // 等待DMA发送完成（可选）
//        while(tx_busy_2) {
//            Delay_ms(1);
//        }
//    } else {
//        // DMA忙，使用查询发送作为后备
//        for(uint16_t i = 0; i < length; i++) {
//            while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
//            USART_SendData(USART2, array[i]);
//        }
//    }
//}

//// 检查DMA发送状态
//uint8_t Serial_DMA_Busy_1(void)
//{
//    return tx_busy_2;
//}

//// 其他辅助函数保持不变
//void Serial_SendHexByte_1(uint8_t byte)
//{
//    uint8_t nibbles[2];
//    nibbles[0] = (byte >> 4) & 0x0F;
//    nibbles[1] = byte & 0x0F;
//    
//    for(int i = 0; i < 2; i++) {
//        if(nibbles[i] < 10) {
//            Serial_SendByte_1('0' + nibbles[i]);
//        } else {
//            Serial_SendByte_1('A' + (nibbles[i] - 10));
//        }
//    }
//}

//void Serial_SendHex_1(uint16_t num)
//{
//    char hex_chars[] = "0123456789ABCDEF";
//    Serial_SendByte_1(hex_chars[(num >> 12) & 0x0F]);
//    Serial_SendByte_1(hex_chars[(num >> 8) & 0x0F]);
//    Serial_SendByte_1(hex_chars[(num >> 4) & 0x0F]);
//    Serial_SendByte_1(hex_chars[num & 0x0F]);
//}

//void Serial_SendString_1(char *string)
//{
//    for(uint16_t i = 0; string[i] != '\0'; i++) {
//        Serial_SendByte_1(string[i]);
//    }
//}

//uint32_t Serial_pow_1(uint32_t x, uint32_t y)
//{
//    uint32_t result = 1;
//    while(y--) {
//        result *= x;
//    }
//    return result;
//}

//void Serial_SendNum_1(uint32_t Num, uint16_t length)
//{
//    for(uint32_t i = 0; i < length; i++) {
//        Serial_SendByte_1(Num / Serial_pow_1(10, length - i - 1) % 10 + '0');
//    }
//}

//int fputc_1(int ch, FILE *f)
//{
//    Serial_SendByte_1(ch);
//    return ch;
//}


