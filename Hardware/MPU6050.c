#include "stm32f10x.h"
#include "stm32f10x_dma.h"
#include "stm32f10x_i2c.h"
#include "MPU6050.h"
// 全局变量
typedef struct {
    volatile uint8_t conv_complete;
    uint8_t rx_buf[2];
    uint8_t tx_buf[3];
    DMA_Channel_TypeDef* dma_ch;
    I2C_TypeDef* i2c;
    uint8_t addr;
} ADS1115_Device;

static ADS1115_Device ads1115[2] = {
    {1, {0}, {0}, DMA1_Channel6, I2C1, ADS1115_ADDR1},  // I2C1
    {1, {0}, {0}, DMA1_Channel4, I2C2, ADS1115_ADDR2}   // I2C2
};

// I2C和DMA初始化
void ADS1115_Dual_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct;
    I2C_InitTypeDef I2C_InitStruct;
    DMA_InitTypeDef DMA_InitStruct;
    NVIC_InitTypeDef NVIC_InitStruct;

    // 1. 初始化I2C1 GPIO (PB6/PB7)
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStruct);

    // 2. 初始化I2C2 GPIO (PB10/PB11)
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
    GPIO_Init(GPIOB, &GPIO_InitStruct);

    // 3. 初始化I2C1
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
    I2C_InitStruct.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStruct.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStruct.I2C_OwnAddress1 = 0x00;
    I2C_InitStruct.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStruct.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStruct.I2C_ClockSpeed = 400000;
    I2C_Init(I2C1, &I2C_InitStruct);
    I2C_Cmd(I2C1, ENABLE);

    // 4. 初始化I2C2
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);
    I2C_Init(I2C2, &I2C_InitStruct);
    I2C_Cmd(I2C2, ENABLE);

    // 5. 初始化DMA1通道6 (I2C1_RX)
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    DMA_DeInit(DMA1_Channel6);
    DMA_InitStruct.DMA_PeripheralBaseAddr = (uint32_t)&(I2C1->DR);
    DMA_InitStruct.DMA_MemoryBaseAddr = (uint32_t)ads1115[0].rx_buf;
    DMA_InitStruct.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStruct.DMA_BufferSize = 2;
    DMA_InitStruct.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStruct.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStruct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStruct.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStruct.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStruct.DMA_Priority = DMA_Priority_High;
    DMA_InitStruct.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel6, &DMA_InitStruct);

    // 6. 初始化DMA1通道4 (I2C2_RX)
    DMA_DeInit(DMA1_Channel4);
    DMA_InitStruct.DMA_PeripheralBaseAddr = (uint32_t)&(I2C2->DR);
    DMA_InitStruct.DMA_MemoryBaseAddr = (uint32_t)ads1115[1].rx_buf;
    DMA_Init(DMA1_Channel4, &DMA_InitStruct);

    // 7. 配置DMA中断
    NVIC_InitStruct.NVIC_IRQChannel = DMA1_Channel6_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
    DMA_ITConfig(DMA1_Channel6, DMA_IT_TC, ENABLE);

    NVIC_InitStruct.NVIC_IRQChannel = DMA1_Channel4_IRQn;
    NVIC_Init(&NVIC_InitStruct);
    DMA_ITConfig(DMA1_Channel4, DMA_IT_TC, ENABLE);
}

// 启动指定通道的单次转换
uint8_t ADS1115_StartConversion(uint8_t i2c_num, ADS1115_Channel ch) {
    if(i2c_num > 1) return 0;
    if(!ads1115[i2c_num].conv_complete) return 0;
    
    // 准备配置命令
    ads1115[i2c_num].tx_buf[0] = 0x01; // 配置寄存器地址
    ads1115[i2c_num].tx_buf[1] = 0xC1 | (ch << 4); // 启动转换+通道选择
    ads1115[i2c_num].tx_buf[2] = (ADS1115_GAIN << 1) | 0x80 | (ADS1115_DR << 5);
    
    // 发送配置命令
    I2C_GenerateSTART(ads1115[i2c_num].i2c, ENABLE);
    while(!I2C_CheckEvent(ads1115[i2c_num].i2c, I2C_EVENT_MASTER_MODE_SELECT));
    
    I2C_Send7bitAddress(ads1115[i2c_num].i2c, ads1115[i2c_num].addr << 1, I2C_Direction_Transmitter);
    while(!I2C_CheckEvent(ads1115[i2c_num].i2c, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
    
    for(uint8_t i=0; i<3; i++) {
        I2C_SendData(ads1115[i2c_num].i2c, ads1115[i2c_num].tx_buf[i]);
        while(!I2C_CheckEvent(ads1115[i2c_num].i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
    }
    I2C_GenerateSTOP(ads1115[i2c_num].i2c, ENABLE);
    
    ads1115[i2c_num].conv_complete = 0;
    return 1;
}

// 检查转换是否完成
uint8_t ADS1115_IsConversionComplete(uint8_t i2c_num) {
    if(i2c_num > 1) return 0;
    return ads1115[i2c_num].conv_complete;
}

// 获取转换结果
int16_t ADS1115_GetConversionResult(uint8_t i2c_num) {
    if(i2c_num > 1) return 0;
    return (ads1115[i2c_num].rx_buf[0] << 8) | ads1115[i2c_num].rx_buf[1];
}

// 直接获取电压值(V)
float ADS1115_GetVoltage(uint8_t i2c_num, ADS1115_Channel ch) {
    if(!ADS1115_StartConversion(i2c_num, ch)) return -1.0f;
    while(!ADS1115_IsConversionComplete(i2c_num));
    int16_t adc_val = ADS1115_GetConversionResult(i2c_num);
    return (adc_val * 4.096f) / 32768.0f;
}

// DMA中断服务函数
void DMA1_Channel6_IRQHandler(void) {
    if(DMA_GetITStatus(DMA1_IT_TC6)) {
        DMA_ClearITPendingBit(DMA1_IT_TC6);
        
        DMA_Cmd(DMA1_Channel6, DISABLE);
        I2C_DMACmd(I2C1, DISABLE);
        
        I2C_AcknowledgeConfig(I2C1, DISABLE);
        I2C_GenerateSTOP(I2C1, ENABLE);
        I2C_AcknowledgeConfig(I2C1, ENABLE);
        
        ads1115[0].conv_complete = 1;
    }
}

void DMA1_Channel4_IRQHandler(void) {
    if(DMA_GetITStatus(DMA1_IT_TC4)) {
        DMA_ClearITPendingBit(DMA1_IT_TC4);
        
        DMA_Cmd(DMA1_Channel4, DISABLE);
        I2C_DMACmd(I2C2, DISABLE);
        
        I2C_AcknowledgeConfig(I2C2, DISABLE);
        I2C_GenerateSTOP(I2C2, ENABLE);
        I2C_AcknowledgeConfig(I2C2, ENABLE);
        
        ads1115[1].conv_complete = 1;
    }
}
