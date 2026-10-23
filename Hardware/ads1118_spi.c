#include "ads1118_spi.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_spi.h"
#include "Delay.h"

#define ADS1118_CS_PORT       GPIOB
#define ADS1118_CS_N_O1_PIN   GPIO_Pin_0
#define ADS1118_CS_N_O2_PIN   GPIO_Pin_1
#define ADS1118_CS_N_O3_PIN   GPIO_Pin_10
#define ADS1118_ALL_CS_PINS   (ADS1118_CS_N_O1_PIN | ADS1118_CS_N_O2_PIN | ADS1118_CS_N_O3_PIN)

#define ADS1118_SPI_TIMEOUT   100000UL

volatile uint8_t  ads1118_spi_ready = 0U;
volatile uint32_t ads1118_spi_timeout_count = 0U;
volatile uint8_t  ads1118_last_chip = 0xFFU;
volatile uint8_t  ads1118_last_channel = 0xFFU;
volatile uint16_t ads1118_last_config = 0U;
volatile uint16_t ads1118_last_raw = 0U;

static void ADS1118_AllCSHigh(void)
{
    /* High = not selected. Never leave two ADCs selected on the shared bus. */
    GPIO_SetBits(ADS1118_CS_PORT, ADS1118_ALL_CS_PINS);
}

static uint8_t ADS1118_Select(uint8_t chip)
{
    ADS1118_AllCSHigh();

    switch (chip)
    {
        case ADS1118_CHIP_N_O1:
            GPIO_ResetBits(ADS1118_CS_PORT, ADS1118_CS_N_O1_PIN);
            return 1U;

        case ADS1118_CHIP_N_O2:
            GPIO_ResetBits(ADS1118_CS_PORT, ADS1118_CS_N_O2_PIN);
            return 1U;

        case ADS1118_CHIP_N_O3:
            GPIO_ResetBits(ADS1118_CS_PORT, ADS1118_CS_N_O3_PIN);
            return 1U;

        default:
            return 0U;
    }
}

static uint8_t ADS1118_TransferByte(uint8_t tx, uint8_t *rx)
{
    uint32_t timeout;

    timeout = ADS1118_SPI_TIMEOUT;
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET)
    {
        if (timeout-- == 0U)
        {
            return 0U;
        }
    }

    SPI_I2S_SendData(SPI1, tx);

    timeout = ADS1118_SPI_TIMEOUT;
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET)
    {
        if (timeout-- == 0U)
        {
            return 0U;
        }
    }

    *rx = (uint8_t)SPI_I2S_ReceiveData(SPI1);
    return 1U;
}

/*
 * ADS1118 supports a 16-clock data cycle: the conversion result is shifted
 * out while the word on DIN is interpreted as a configuration/NOP word.
 * The CS line is raised after each 16-bit transfer to reset the SPI frame.
 */
static uint8_t ADS1118_Transfer16(uint8_t chip, uint16_t tx_word, uint16_t *rx_word)
{
    uint8_t rx_hi;
    uint8_t rx_lo;
    uint32_t timeout;

    rx_hi = 0U;
    rx_lo = 0U;

    if (ADS1118_Select(chip) == 0U)
    {
        ads1118_spi_timeout_count++;
        return 0U;
    }

    Delay_us(2U);

    if (ADS1118_TransferByte((uint8_t)(tx_word >> 8), &rx_hi) == 0U)
    {
        goto transfer_error;
    }

    if (ADS1118_TransferByte((uint8_t)(tx_word & 0x00FFU), &rx_lo) == 0U)
    {
        goto transfer_error;
    }

    timeout = ADS1118_SPI_TIMEOUT;
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET)
    {
        if (timeout-- == 0U)
        {
            goto transfer_error;
        }
    }

    Delay_us(1U);
    ADS1118_AllCSHigh();

    *rx_word = (uint16_t)(((uint16_t)rx_hi << 8) | (uint16_t)rx_lo);
    return 1U;

transfer_error:
    /* Drain a pending byte, if any, before releasing the selected device. */
    if (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) != RESET)
    {
        (void)SPI_I2S_ReceiveData(SPI1);
    }
    ADS1118_AllCSHigh();
    ads1118_spi_timeout_count++;
    return 0U;
}

void ADS1118_Init(void)
{
    GPIO_InitTypeDef gpio;
    SPI_InitTypeDef spi;

    ads1118_spi_ready = 0U;
    ads1118_spi_timeout_count = 0U;
    ads1118_last_chip = 0xFFU;
    ads1118_last_channel = 0xFFU;
    ads1118_last_config = 0U;
    ads1118_last_raw = 0U;

    RCC_APB2PeriphClockCmd(
        RCC_APB2Periph_AFIO |
        RCC_APB2Periph_GPIOA |
        RCC_APB2Periph_GPIOB |
        RCC_APB2Periph_SPI1,
        ENABLE);

    /* CS: PB0=N_O1, PB1=N_O2, PB10=N_O3; all are active-low. */
    gpio.GPIO_Pin = ADS1118_ALL_CS_PINS;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(ADS1118_CS_PORT, &gpio);
    ADS1118_AllCSHigh();

    /* SPI1 SCK=PA5 and MOSI=PA7 are alternate-function push-pull outputs. */
    gpio.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    /* SPI1 MISO=PA6 is the ADS1118 DOUT/DRDY signal. */
    gpio.GPIO_Pin = GPIO_Pin_6;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    SPI_I2S_DeInit(SPI1);
    spi.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    spi.SPI_Mode = SPI_Mode_Master;
    spi.SPI_DataSize = SPI_DataSize_8b;
    spi.SPI_CPOL = SPI_CPOL_Low;
    spi.SPI_CPHA = SPI_CPHA_2Edge;
    spi.SPI_NSS = SPI_NSS_Soft;
    spi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_32;
    spi.SPI_FirstBit = SPI_FirstBit_MSB;
    spi.SPI_CRCPolynomial = 7U;
    SPI_Init(SPI1, &spi);
    SPI_Cmd(SPI1, ENABLE);

    ads1118_spi_ready = 1U;
}

uint8_t ADS1118_ReadChannel(uint8_t chip, uint8_t channel, int16_t *raw)
{
    uint16_t config;
    uint16_t ignored_result;
    uint16_t conversion_result;

    if ((ads1118_spi_ready == 0U) ||
        (raw == 0) ||
        (chip > ADS1118_CHIP_N_O3) ||
        (channel > 3U))
    {
        return 0U;
    }

    ads1118_last_chip = chip;
    ads1118_last_channel = channel;

    /*
     * Config: OS=1 (start single-shot), MUX=AINx-to-GND, PGA=+/-2.048 V,
     * single-shot mode, 860 SPS, internal temperature mode disabled.
     * 0xC5EB, 0xD5EB, 0xE5EB, 0xF5EB select AIN0..AIN3 respectively.
     */
    config = (uint16_t)(0xC5EBU + ((uint16_t)channel << 12));

    /* First 16-bit transfer writes the new channel configuration. */
    if (ADS1118_Transfer16(chip, config, &ignored_result) == 0U)
    {
        return 0U;
    }
    ads1118_last_config = config;

    /* 860 SPS conversion time is about 1.16 ms; 2 ms leaves margin. */
    Delay_ms(2U);

    /*
     * NOP bits [2:1] = 00 in 0x0000, so this transfer reads the buffered
     * conversion result without overwriting the configuration register.
     */
    if (ADS1118_Transfer16(chip, 0x0000U, &conversion_result) == 0U)
    {
        return 0U;
    }

    ads1118_last_raw = conversion_result;
    *raw = (int16_t)conversion_result;
    return 1U;
}
