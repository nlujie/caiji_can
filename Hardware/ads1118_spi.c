#include "ads1118_spi.h"
#include "stm32f10x_spi.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "serial.h"
#include "delay.h"

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
    u8  Hbyte=0;
    u8  Lbyte=0;
    u16 tmp=0;

    ADC_CS1_Low(); // 拉低PA4片选
    Delay_us(8);
    
    Hbyte = ADS1118_ReadWrite_SPI1((u8)(ConfigReg>>8));
    Lbyte = ADS1118_ReadWrite_SPI1((u8)ConfigReg);
    
    
    
    
//    // 重要：等待转换完成
//    // 对于128SPS，等待8ms；对于475SPS，等待2.1ms
//    Delay_ms(10); // 增加足够的等待时间
//    
//    // 再次读取获取转换结果
//    Hbyte = ADS1118_ReadWrite_SPI1((u8)(ConfigReg>>8));
//    Lbyte = ADS1118_ReadWrite_SPI1((u8)ConfigReg);
  
    
    
    
    
    
    
    tmp = ((u16)Hbyte<<8)|(u16)Lbyte;
    
    Delay_us(8);
    ADC_CS1_High(); // 拉高PA4片选
    
    return tmp;
}

// SPI1第二个芯片读取函数 (PA8片选)
u16 ADS1118_ReadVotage_SPI1_Chip2(u16 ConfigReg)
{  
    u8  Hbyte=0;
    u8  Lbyte=0;
    u16 tmp=0;

    ADC_CS2_Low(); // 拉低PA8片选
    Delay_us(8);
    
    Hbyte = ADS1118_ReadWrite_SPI1((u8)(ConfigReg>>8));
    Lbyte = ADS1118_ReadWrite_SPI1((u8)ConfigReg);
    tmp = ((u16)Hbyte<<8)|(u16)Lbyte;
    
    Delay_us(8);
    ADC_CS2_High(); // 拉高PA8片选
    
    return tmp;
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
    static u16 last_config = 0;
    u8 Hbyte, Lbyte;
    u16 tmp;

    ADC_CS1_Low();
    Delay_us(15);  // 增加片选建立时间
    
    // 检查是否需要切换通道
    if ((last_config & 0x7000) != (ConfigReg & 0x7000)) {
        // 通道切换，需要重新配置
        (void)ADS1118_ReadWrite_SPI1((u8)(ConfigReg >> 8));
        (void)ADS1118_ReadWrite_SPI1((u8)ConfigReg);
        last_config = ConfigReg;
        
        // 通道切换后需要更长的稳定时间
        ADC_CS1_High();
        Delay_ms(15);  // 增加稳定时间
        ADC_CS1_Low();
        Delay_us(15);
        
        // 读取并丢弃第一个数据（确保是当前通道的数据）
        (void)ADS1118_ReadWrite_SPI1(0xFF);
        (void)ADS1118_ReadWrite_SPI1(0xFF);
        
        // 再等待一次转换周期
        ADC_CS1_High();
        Delay_ms(8);   // 128SPS转换时间
        ADC_CS1_Low();
        Delay_us(15);
    }
    
    // 读取当前转换结果
    Hbyte = ADS1118_ReadWrite_SPI1((u8)(ConfigReg >> 8));
    Lbyte = ADS1118_ReadWrite_SPI1((u8)ConfigReg);
    tmp = ((u16)Hbyte << 8) | (u16)Lbyte;
    
    Delay_us(10);
    ADC_CS1_High();
    
    return tmp;
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
    GPIO_InitTypeDef GPIO_InitStructure;
    SPI_InitTypeDef SPI_InitStructure;
    
    // 开启 GPIOA 和 SPI1 时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
    
    // SPI1引脚：PA5-SCK、PA7-MOSI为复用推挽；PA6-MISO为浮空输入
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_SetBits(GPIOA, GPIO_Pin_5 | GPIO_Pin_7);
    
    // SPI1 参数配置
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;// 全双工
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;// 主机模式
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;// 8位数据宽度
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;// 空闲时钟低电平
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;// 第二个边沿采样
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;// 软件管理片选
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_64; // 约1.125MHz，降低ISO7741/PCB连线时序风险
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;// 高位先发
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_Init(SPI1, &SPI_InitStructure);
 
    SPI_Cmd(SPI1, ENABLE);// 使能 SPI1
    
    /* ADC转换在首次读取时按需启动；三个芯片共用SPI1。 */
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
    GPIO_InitTypeDef GPIO_InitStructure;

    /* 采集板使用说明：N_O1=PB0、N_O2=PB1、N_O3=PB10，三片ADS1118共用SPI1。 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    ADC_CS1_High();
    ADC_CS2_High();
    ADC_CS3_High();

    SPI1_Init();
    SPI1_SetSpeed(SPI_BaudRatePrescaler_64);
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

u8 ADS1118_ReadWrite_SPI1(u8 ConfigReg1)
{
    u8 retry=0;
    // 等待发送缓冲区空
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET)
    {
        retry++;
        if(retry>200)return 0;
    }																														
    
    SPI_I2S_SendData(SPI1, ConfigReg1);  // 发送1字节
    
    // 等待接收完成
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET)
    {
        retry++;
        if(retry>200)return 0;
    }	
    
    return SPI_I2S_ReceiveData(SPI1);// 读取接收到的数据
}

/*
 * 原理图：三片ADS1118共用SPI1，N_O1/2/3分别由PB0/PB1/PB10经PC817驱动。
 * 采用TI ADS1118数据手册9.5.7.1的32-bit格式读取Config回显检查连通性。
 */
/* -------------------------------------------------------------------------
 * Board: 3 x ADS1118, SPI1 shared, PB0(N_O1), PB1(N_O2), PB10(N_O3).
 * N_O1 uses ISO7741; N_O2/N_O3 use PC817 in low-active select circuits.
 * Datasheet 9.5.7: 16 clocks can start a single-shot conversion and 16 more
 * clocks in a separate /CS transaction can read its completed conversion.
 * The second transaction sends NOP 0x0000, NOT another conversion config.
 * Use 32-clock config readback for *diagnosis*, not as a hard data gate.
 * ------------------------------------------------------------------------- */
volatile u8 ads1118_drdy_before_read[3] = {0U, 0U, 0U};

static void ADS1118_CS_AllHigh(void)
{
    ADC_CS1_High();
    ADC_CS2_High();
    ADC_CS3_High();
}

static u8 ADS1118_SPI1_ExchangeChecked(u8 tx, u8 *rx)
{
    u32 timeout = 30000U;
    while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET)
    {
        if(--timeout == 0U) return 0U;
    }
    SPI_I2S_SendData(SPI1, tx);
    timeout = 30000U;
    while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET)
    {
        if(--timeout == 0U) return 0U;
    }
    *rx = (u8)SPI_I2S_ReceiveData(SPI1);
    return 1U;
}

static u8 ADS1118_SPI1_Frame(u8 chip, u16 outWord, u16 *inWord,
                              u8 *readyLow)
{
    u8 rxHi = 0U, rxLo = 0U;
    u8 ok = 1U;
    u32 timeout;
    volatile u16 discard;

    if(chip < 1U || chip > 3U || inWord == 0) return 0U;
    if(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_OVR) == SET)
    {
        discard = SPI1->DR;
        discard = SPI1->SR;
        (void)discard;
    }

    ADS1118_CS_AllHigh();
    Delay_us(60); /* 光耦与隔离输出完成恢复 */
    if(chip == 1U) ADC_CS1_Low();
    else if(chip == 2U) ADC_CS2_Low();
    else ADC_CS3_Low();
    Delay_us(60); /* PC817低电平有效片选稳定后再发时钟 */

    /* DOUT/DRDY is readable on PA6 when /CS is low, before the clock train. */
    if(readyLow != 0)
        *readyLow = (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_6) == Bit_RESET) ? 1U : 0U;

    ok = ADS1118_SPI1_ExchangeChecked((u8)(outWord >> 8), &rxHi);
    if(ok) ok = ADS1118_SPI1_ExchangeChecked((u8)outWord, &rxLo);

    timeout = 30000U;
    while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET)
    {
        if(--timeout == 0U) { ok = 0U; break; }
    }
    ADS1118_CS_AllHigh();
    Delay_us(60);

    *inWord = ((u16)rxHi << 8) | (u16)rxLo;
    return ok;
}

/* 32-clock exchange only for independent config register readback diagnostics. */
static u8 ADS1118_SPI1_CheckEcho(u8 chip, u16 cfg, u16 *echoOut)
{
    u16 oldResult, echo;
    u8 data[4] = {0U, 0U, 0U, 0U};
    u8 ok = 1U;
    u32 timeout;
    if(chip < 1U || chip > 3U || echoOut == 0) return 0U;
    *echoOut = 0U;
    ADS1118_CS_AllHigh();
    Delay_us(60);
    if(chip == 1U) ADC_CS1_Low();
    else if(chip == 2U) ADC_CS2_Low();
    else ADC_CS3_Low();
    Delay_us(60);

    ok = ADS1118_SPI1_ExchangeChecked((u8)(cfg >> 8), &data[0]);
    if(ok) ok = ADS1118_SPI1_ExchangeChecked((u8)cfg, &data[1]);
    if(ok) ok = ADS1118_SPI1_ExchangeChecked(0x00U, &data[2]);
    if(ok) ok = ADS1118_SPI1_ExchangeChecked(0x00U, &data[3]);
    timeout = 30000U;
    while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET)
    {
        if(--timeout == 0U) { ok = 0U; break; }
    }
    ADS1118_CS_AllHigh();
    Delay_us(60);
    oldResult = ((u16)data[0] << 8) | (u16)data[1];
    (void)oldResult;
    echo = ((u16)data[2] << 8) | (u16)data[3];
    *echoOut = echo;
    return ok;
}

/* return 0=transport error/stuck bus, 1=sample and config verified,
 *        2=sample read but config echo disagrees (untrusted measurement).
 */
u8 ADS1118_ReadChannelChecked_SPI1(u8 chip, u8 channel,
                                   u16 *raw, u16 *last_config_echo)
{
    u16 cfg;
    u16 discarded = 0U;
    u16 sample = 0U;
    u16 echo = 0U;
    u8 attempt;
    u8 drdyLow = 0U;
    u8 echoTransferred;
    u8 echoMatches;

    if(raw == 0 || last_config_echo == 0 ||
       chip < 1U || chip > 3U || channel > 3U) return 0U;

    cfg = (u16)(ADS1118_CONFIG_AIN0 + ((u16)channel << 12));
    *raw = 0U;
    *last_config_echo = 0U;
    ads1118_drdy_before_read[chip - 1U] = 0U;

    for(attempt = 0U; attempt < 2U; attempt++)
    {
        /* Single-shot start, 860SPS, PGA +/-2.048V. */
        if(!ADS1118_SPI1_Frame(chip, cfg, &discarded, 0)) continue;
        Delay_ms(4);
        /* Get completed sample; send NOP, so no new conversion is started. */
        if(!ADS1118_SPI1_Frame(chip, 0x0000U, &sample, &drdyLow)) continue;
        ads1118_drdy_before_read[chip - 1U] = drdyLow;
        *raw = sample;

        /* Diagnostic echo uses an independent 32-clock command and does
           not overwrite the sample already captured above. */
        echoTransferred = ADS1118_SPI1_CheckEcho(chip, cfg, &echo);
        *last_config_echo = echo;
        echoMatches = (echoTransferred &&
                      ((echo & 0x7FFFU) == (cfg & 0x7FFFU))) ? 1U : 0U;

        /* FFFF/0000 simultaneously on raw and config readback strongly
           indicates an undriven MISO or failed isolation/power. */
        if((sample == 0xFFFFU && echo == 0xFFFFU) ||
           (sample == 0x0000U && echo == 0x0000U && !drdyLow))
            continue;

        if(echoMatches) return 1U;
        return 2U; /* Preserve true raw value for CAN diagnostics. */
    }
    return 0U;
}

u16 ADS1118_ReadChannel_SPI1(u8 chip, u8 channel)
{
    u16 raw = 0U, echo = 0U;
    (void)ADS1118_ReadChannelChecked_SPI1(chip, channel, &raw, &echo);
    return raw;
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
    u8  Hbyte=0;
    u8  Lbyte=0;
    u16 tmp=0;

    ADC_CS3_Low();
    Delay_us(2);
    
    Hbyte = ADS1118_ReadWrite_SPI2((u8)(ConfigReg>>8));
    Lbyte = ADS1118_ReadWrite_SPI2((u8)ConfigReg);
    tmp = ((u16)Hbyte<<8)|(u16)Lbyte;
    
    Delay_us(2);
    ADC_CS3_High();

    return tmp;
}

