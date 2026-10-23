#include "ads1118_spi.h"
#include "stm32f10x_spi.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "serial.h"
#include "delay.h"

/* Hardware diagnostic counters, visible in Keil Watch. */
volatile u32 ads1118_spi_timeout_count = 0;
volatile u32 ads1118_conversion_count = 0;

/* Single-shot transaction:
 * 1. Write channel config (16 SCLK, CS high ends frame).
 * 2. Wait > 1 conversion at 128 SPS.
 * 3. Read conversion with 0x0000 dummy (NOP, leaves config unchanged).
 * All ADCs are on SPI1; only chip-select differs.
 */
static u16 ADS1118_ReadOnSPI1(GPIO_TypeDef *cs_port, u16 cs_pin, u16 config)
{
    u8 hi, lo;
    u32 remaining;
    u32 failures = ads1118_spi_timeout_count;

    GPIO_ResetBits(cs_port, cs_pin);
    Delay_us(4);
    (void)ADS1118_ReadWrite_SPI1((u8)(config >> 8));
    (void)ADS1118_ReadWrite_SPI1((u8)config);
    remaining = 100000U;
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET) {
        if (--remaining == 0U) { ads1118_spi_timeout_count++; break; }
    }
    GPIO_SetBits(cs_port, cs_pin);
    if (ads1118_spi_timeout_count != failures) return 0xFFFFU;

    /* 128 SPS nominal conversion 7.8125 ms; allow oscillator tolerance. */
    Delay_ms(10);
    GPIO_ResetBits(cs_port, cs_pin);
    Delay_us(4);
    hi = ADS1118_ReadWrite_SPI1(0x00U);
    lo = ADS1118_ReadWrite_SPI1(0x00U);
    remaining = 100000U;
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET) {
        if (--remaining == 0U) { ads1118_spi_timeout_count++; break; }
    }
    GPIO_SetBits(cs_port, cs_pin);
    if (ads1118_spi_timeout_count != failures) return 0xFFFFU;
    ads1118_conversion_count++;
    return (u16)(((u16)hi << 8) | lo);
}


// 连续读取模式 - 更高效
//u16 ADS1118_ReadVotage_SPI1_Chip1(u16 ConfigReg)
//{  
//    static u16 last_config = 0;
//    u8  Hbyte = 0;
//    u8  Lbyte = 0;
//    u16 tmp = 0;

//    ADC_CS1_Low();
//    Delay_us(2);
//    
//    // 如果配置改变，需要重新启动转换
//    if (last_config != ConfigReg) {
//        // 发送新配置启动转换
//        (void)ADS1118_ReadWrite_SPI1((u8)(ConfigReg >> 8));
//        (void)ADS1118_ReadWrite_SPI1((u8)ConfigReg);
//        last_config = ConfigReg;
//        
//        // 等待第一次转换完成
//        Delay_ms(10);
//    }
//    
//    // 读取当前转换结果（同时启动下一次转换）
//    Hbyte = ADS1118_ReadWrite_SPI1((u8)(ConfigReg >> 8));
//    Lbyte = ADS1118_ReadWrite_SPI1((u8)ConfigReg);
//    tmp = ((u16)Hbyte << 8) | (u16)Lbyte;
//    
//    Delay_us(2);
//    ADC_CS1_High();
//    
//    return tmp;
//}

// SPI1第一个芯片读取函数 (PA4片选)
u16 ADS1118_ReadVotage_SPI1_Chip1(u16 ConfigReg)
{
    return ADS1118_ReadOnSPI1(GPIOB, GPIO_Pin_0, ConfigReg);
}

// SPI1第二个芯片读取函数 (PA8片选)
u16 ADS1118_ReadVotage_SPI1_Chip2(u16 ConfigReg)
{
    return ADS1118_ReadOnSPI1(GPIOB, GPIO_Pin_1, ConfigReg);
}



//// 连续转换模式下的优化读取函数
//u16 ADS1118_ReadVotage_SPI1_Chip1(u16 ConfigReg)
//{  
//    static u16 last_config = 0;
//    u8 Hbyte, Lbyte;
//    u16 tmp;

//    ADC_CS1_Low();
//    Delay_us(10);
//    
//    // 检查是否需要切换通道
//    if (last_config != ConfigReg) {
//        // 通道切换，需要发送新配置
//        (void)ADS1118_ReadWrite_SPI1((u8)(ConfigReg >> 8));
//        (void)ADS1118_ReadWrite_SPI1((u8)ConfigReg);
//        last_config = ConfigReg;
//        
//        // 在连续模式下，切换通道后需要丢弃第一个读数
//        ADC_CS1_High();
//        Delay_ms(10);  // 等待新通道稳定
//        ADC_CS1_Low();
//        Delay_us(10);
//        
//        // 读取并丢弃第一个数据
//        (void)ADS1118_ReadWrite_SPI1(0xFF);
//        (void)ADS1118_ReadWrite_SPI1(0xFF);
//    }
//    
//    // 读取当前转换结果
//    Hbyte = ADS1118_ReadWrite_SPI1((u8)(ConfigReg >> 8));
//    Lbyte = ADS1118_ReadWrite_SPI1((u8)ConfigReg);
//    tmp = ((u16)Hbyte << 8) | (u16)Lbyte;
//    
//    Delay_us(5);
//    ADC_CS1_High();
//    
//    return tmp;
//}

//// SPI1第二个芯片读取函数 (PA8片选) - 同样优化
//u16 ADS1118_ReadVotage_SPI1_Chip2(u16 ConfigReg)
//{  
//    static u16 last_config = 0;
//    u8 Hbyte, Lbyte;
//    u16 tmp;

//    ADC_CS2_Low();
//    Delay_us(10);
//    
//    // 检查是否需要切换通道
//    if (last_config != ConfigReg) {
//        // 通道切换，需要发送新配置
//        (void)ADS1118_ReadWrite_SPI1((u8)(ConfigReg >> 8));
//        (void)ADS1118_ReadWrite_SPI1((u8)ConfigReg);
//        last_config = ConfigReg;
//        
//        // 在连续模式下，切换通道后需要丢弃第一个读数
//        ADC_CS2_High();
//        Delay_ms(10);  // 等待新通道稳定
//        ADC_CS2_Low();
//        Delay_us(10);
//        
//        // 读取并丢弃第一个数据
//        (void)ADS1118_ReadWrite_SPI1(0xFF);
//        (void)ADS1118_ReadWrite_SPI1(0xFF);
//    }
//    
//    // 读取当前转换结果
//    Hbyte = ADS1118_ReadWrite_SPI1((u8)(ConfigReg >> 8));
//    Lbyte = ADS1118_ReadWrite_SPI1((u8)ConfigReg);
//    tmp = ((u16)Hbyte << 8) | (u16)Lbyte;
//    
//    Delay_us(5);
//    ADC_CS2_High();
//    
//    return tmp;
//}



//// 优化的单次读取函数 - 确保每次读取都是正确的数据
u16 ADS1118_ReadVotage_SPI1_Chip1_Optimized(u16 ConfigReg)
{
    return ADS1118_ReadVotage_SPI1_Chip1(ConfigReg);
}

//// 同样的优化应用于第二个芯片
//u16 ADS1118_ReadVotage_SPI1_Chip2_Optimized(u16 ConfigReg)
//{  
//    static u16 last_config = 0;
//    u8 Hbyte, Lbyte;
//    u16 tmp;

//    ADC_CS2_Low();
//    Delay_us(15);
//    
//    if ((last_config & 0x7000) != (ConfigReg & 0x7000)) {
//        (void)ADS1118_ReadWrite_SPI1((u8)(ConfigReg >> 8));
//        (void)ADS1118_ReadWrite_SPI1((u8)ConfigReg);
//        last_config = ConfigReg;
//        
//        ADC_CS2_High();
//        Delay_ms(15);
//        ADC_CS2_Low();
//        Delay_us(15);
//        
//        (void)ADS1118_ReadWrite_SPI1(0xFF);
//        (void)ADS1118_ReadWrite_SPI1(0xFF);
//        
//        ADC_CS2_High();
//        Delay_ms(8);
//        ADC_CS2_Low();
//        Delay_us(15);
//    }
//    
//    Hbyte = ADS1118_ReadWrite_SPI1((u8)(ConfigReg >> 8));
//    Lbyte = ADS1118_ReadWrite_SPI1((u8)ConfigReg);
//    tmp = ((u16)Hbyte << 8) | (u16)Lbyte;
//    
//    Delay_us(10);
//    ADC_CS2_High();
//    
//    return tmp;
//}




// 旧的SPI1读取函数（为了兼容性保留）
u16 ADS1118_ReadVotage_SPI1(u16 ConfigReg)
{  
    return ADS1118_ReadVotage_SPI1_Chip1(ConfigReg); // 默认使用第一个芯片
}

void SPI1_Init(void)
{
    GPIO_InitTypeDef gpio;
    SPI_InitTypeDef spi;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_SPI1, ENABLE);

    /* STM32F103 SPI1 pins: PA5=SCK, PA6=MISO (INPUT), PA7=MOSI. */
    gpio.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);
    gpio.GPIO_Pin = GPIO_Pin_6;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio);

    SPI_I2S_DeInit(SPI1);
    SPI_StructInit(&spi);
    spi.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    spi.SPI_Mode = SPI_Mode_Master;
    spi.SPI_DataSize = SPI_DataSize_8b;
    spi.SPI_CPOL = SPI_CPOL_Low;
    spi.SPI_CPHA = SPI_CPHA_2Edge; /* ADS1118 SPI mode 1 */
    spi.SPI_NSS = SPI_NSS_Soft;
    spi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_32; /* 72MHz/32 = 2.25MHz */
    spi.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_Init(SPI1, &spi);
    SPI_Cmd(SPI1, ENABLE);
}

void SPI2_Init(void)
{	
    GPIO_InitTypeDef GPIO_InitStructure;
    SPI_InitTypeDef SPI_InitStructure;
    
    // 开启 GPIOB 和 SPI2 时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
    
    // SPI2引脚: PB13-SCK, PB14-MISO, PB15-MOSI
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;// 复用推挽
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    GPIO_SetBits(GPIOB, GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15);
    
    // SPI2 参数配置（与 SPI1 相同）
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_32;
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_Init(SPI2, &SPI_InitStructure);
 
    SPI_Cmd(SPI2, ENABLE);
    
    // 使用新的2.048V量程配置
    ADS1118_ReadVotage_SPI2(ADS1118_CONFIG_AIN2);//启动传输
    Delay_ms(8);
}

void ADS1118_Init(void)
{
    GPIO_InitTypeDef gpio;

    /* Actual schematic chip selects: PB0=N_O1, PB1=N_O2, PB10=N_O3. */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    GPIO_SetBits(GPIOB, GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_10);
    gpio.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_10;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &gpio);
    ADC_CS1_High();
    ADC_CS2_High();
    ADC_CS3_High();

    /* PCB has three SPI1 ADS1118s, not an ADS1118 on SPI2. */
    SPI1_Init();
    Delay_ms(2);
}

void SPI1_SetSpeed(u8 SPI_BaudRatePrescaler)
{
    assert_param(IS_SPI_BAUDRATE_PRESCALER(SPI_BaudRatePrescaler));
    SPI1->CR1 &= 0XFFC7;  // 清除分频位
    SPI1->CR1 |= SPI_BaudRatePrescaler;
    SPI_Cmd(SPI1,ENABLE); 
}

void SPI2_SetSpeed(u8 SPI_BaudRatePrescaler)
{
    assert_param(IS_SPI_BAUDRATE_PRESCALER(SPI_BaudRatePrescaler));
    SPI2->CR1 &= 0XFFC7;
    SPI2->CR1 |= SPI_BaudRatePrescaler;
    SPI_Cmd(SPI2,ENABLE); 
}

u8 ADS1118_ReadWrite_SPI1(u8 byte_out)
{
    u32 timeout = 100000U;
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET) {
        if (--timeout == 0U) { ads1118_spi_timeout_count++; return 0U; }
    }
    SPI_I2S_SendData(SPI1, byte_out);
    timeout = 100000U;
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET) {
        if (--timeout == 0U) { ads1118_spi_timeout_count++; return 0U; }
    }
    return (u8)SPI_I2S_ReceiveData(SPI1);
}

u8 ADS1118_ReadWrite_SPI2(u8 ConfigReg1)
{
    u8 retry=0;
    while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET)
    {
        retry++;
        if(retry>200)return 0;
    }																														
    
    SPI_I2S_SendData(SPI2, ConfigReg1);
    
    while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET)
    {
        retry++;
        if(retry>200)return 0;
    }	
    
    return SPI_I2S_ReceiveData(SPI2);
}

// SPI2芯片读取函数
u16 ADS1118_ReadVotage_SPI2(u16 ConfigReg)
{
    /* Compatibility with legacy callers: third ADS1118 uses SPI1 and PB10. */
    return ADS1118_ReadOnSPI1(GPIOB, GPIO_Pin_10, ConfigReg);
}

u16 ADS1118_ReadVotage_SPI1_Chip2_Optimized(u16 ConfigReg)
{
    return ADS1118_ReadVotage_SPI1_Chip2(ConfigReg);
}

