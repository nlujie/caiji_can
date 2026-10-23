#include "adc420_ads1118.h"
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_spi.h"

/* ADS1118: OS=1, PGA=+/-2.048V, MODE=single shot, DR=860SPS,
 * TS_MODE=0, NOP=01 (write config), bit0=1.
 * For AIN0..3-to-GND, MUX values are 100..111. */
#define ADS1118_CONFIG_BASE     0x85EBU
#define ADS1118_TIMEOUT_LOOPS   30000UL

/* At 860 SPS conversion is approximately 1.163ms. 3ms includes margin.
 * The chip's DOUT/DRDY is shared with SPI MISO; CS remains high while waiting. */
extern uint32_t ADC420_Millis(void);

static void adc420_select(uint8_t chip, BitAction action)
{
    if (chip == ADC420_CHIP_O1) GPIO_WriteBit(GPIOB, GPIO_Pin_0, action);
    else if (chip == ADC420_CHIP_O2) GPIO_WriteBit(GPIOB, GPIO_Pin_1, action);
    else GPIO_WriteBit(GPIOB, GPIO_Pin_10, action);
}

static uint8_t adc420_spi_byte(uint8_t tx, uint8_t *rx)
{
    uint32_t limit;
    for (limit = ADS1118_TIMEOUT_LOOPS; limit > 0U; --limit) {
        if (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == SET) break;
    }
    if (limit == 0U) return 0U;
    SPI_I2S_SendData(SPI1, tx);

    for (limit = ADS1118_TIMEOUT_LOOPS; limit > 0U; --limit) {
        if (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == SET) break;
    }
    if (limit == 0U) return 0U;
    *rx = (uint8_t)SPI_I2S_ReceiveData(SPI1);
    return 1U;
}

static uint8_t adc420_spi_word(uint16_t tx, uint16_t *rx)
{
    uint8_t high, low;
    if (!adc420_spi_byte((uint8_t)(tx >> 8), &high)) return 0U;
    if (!adc420_spi_byte((uint8_t)tx, &low)) return 0U;
    *rx = ((uint16_t)high << 8) | low;
    return 1U;
}

void ADC420_ADS1118_Init(void)
{
    GPIO_InitTypeDef gpio;
    SPI_InitTypeDef spi;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA |
                           RCC_APB2Periph_GPIOB |
                           RCC_APB2Periph_AFIO |
                           RCC_APB2Periph_SPI1, ENABLE);

    /* Assert no chip select while configuring peripheral. */
    GPIO_SetBits(GPIOB, GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_10);
    gpio.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_10;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &gpio);

    gpio.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7; /* SCK and MOSI */
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &gpio);
    gpio.GPIO_Pin = GPIO_Pin_6;             /* MISO is an input, not AF_PP */
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio);

    SPI_I2S_DeInit(SPI1);
    SPI_StructInit(&spi);
    spi.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    spi.SPI_Mode = SPI_Mode_Master;
    spi.SPI_DataSize = SPI_DataSize_8b;
    spi.SPI_CPOL = SPI_CPOL_Low;
    spi.SPI_CPHA = SPI_CPHA_2Edge;         /* ADS1118 SPI mode 1 */
    spi.SPI_NSS = SPI_NSS_Soft;
    spi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_32; /* 2.25MHz at PCLK2=72MHz */
    spi.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_Init(SPI1, &spi);
    SPI_Cmd(SPI1, ENABLE);
}

uint8_t ADC420_ADS1118_Read(uint8_t chip, uint8_t ain, int16_t *raw)
{
    uint16_t cfg, unused, data;
    uint32_t started;
    if (raw == 0 || chip > ADC420_CHIP_O3 || ain > 3U) return 0U;

    cfg = (uint16_t)(ADS1118_CONFIG_BASE | ((uint16_t)(ain + 4U) << 12));
    /* 16 SCLK cycles, CS high at end. Starts conversion with OS=1. */
    adc420_select(chip, Bit_RESET);
    if (!adc420_spi_word(cfg, &unused)) {
        adc420_select(chip, Bit_SET);
        return 0U;
    }
    adc420_select(chip, Bit_SET);

    /* Wait for the NEW MUX conversion, not the previous buffered result. */
    started = ADC420_Millis();
    while ((uint32_t)(ADC420_Millis() - started) < 3U) { }

    /* NOP=00: read 16-bit conversion without changing the configuration.
     * CS must return high after exactly 16 clocks. */
    adc420_select(chip, Bit_RESET);
    if (!adc420_spi_word(0x0000U, &data)) {
        adc420_select(chip, Bit_SET);
        return 0U;
    }
    adc420_select(chip, Bit_SET);
    *raw = (int16_t)data;
    return 1U;
}
