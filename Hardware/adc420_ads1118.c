#include "adc420_ads1118.h"
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_spi.h"

/* ADS1118: OS=1, +/-2.048V, MODE=single-shot, DR=860 SPS,
 * TS_MODE=0, PULL_UP_EN=1, NOP=01 (write configuration).
 * AIN0..3 to GND have MUX bits = 100..111. */
#define ADC420_CONFIG_BASE      0x85EBU
#define ADC420_CONFIG_TEMP      0xC5FBU
#define ADC420_SPI_TIMEOUT      30000UL
#define ADC420_VERIFY_MASK      0x7FF0U /* excludes self-clearing OS bit */
extern uint32_t ADC420_Millis(void);

/* Keil Watch diagnostics: inspect index 0 for P1 / ADC_4 (N_O1 / PB0).
 * SPI state 1 only after a verified config echo and received conversion. */
volatile uint8_t adc420_diag_spi_state[3];
volatile uint16_t adc420_diag_config_written[3];
volatile uint16_t adc420_diag_config_echo[3];
volatile int16_t adc420_diag_last_raw[3];
volatile uint32_t adc420_diag_read_count[3];
volatile uint8_t adc420_diag_temp_ok_mask;
volatile int16_t adc420_diag_temp_raw[3];

static void adc420_select(uint8_t chip, BitAction action)
{
    if (chip == ADC420_CHIP_O1) GPIO_WriteBit(GPIOB, GPIO_Pin_0, action);
    else if (chip == ADC420_CHIP_O2) GPIO_WriteBit(GPIOB, GPIO_Pin_1, action);
    else GPIO_WriteBit(GPIOB, GPIO_Pin_10, action);
}

static uint8_t adc420_spi_byte(uint8_t tx, uint8_t *rx)
{
    uint32_t count;
    for (count = ADC420_SPI_TIMEOUT; count > 0U; --count) {
        if (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == SET) break;
    }
    if (count == 0U) return 0U;
    SPI_I2S_SendData(SPI1, tx);

    for (count = ADC420_SPI_TIMEOUT; count > 0U; --count) {
        if (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == SET) break;
    }
    if (count == 0U) return 0U;
    *rx = (uint8_t)SPI_I2S_ReceiveData(SPI1);
    return 1U;
}

static uint8_t adc420_spi_word(uint16_t tx, uint16_t *rx)
{
    uint8_t msb, lsb;
    if (!adc420_spi_byte((uint8_t)(tx >> 8), &msb)) return 0U;
    if (!adc420_spi_byte((uint8_t)tx, &lsb)) return 0U;
    *rx = ((uint16_t)msb << 8) | lsb;
    return 1U;
}

static uint8_t adc420_spi_idle(void)
{
    uint32_t count;
    for (count = ADC420_SPI_TIMEOUT; count > 0U; --count) {
        if (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == RESET) return 1U;
    }
    return 0U;
}

void ADC420_ADS1118_Init(void)
{
    GPIO_InitTypeDef gpio;
    SPI_InitTypeDef spi;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |
                           RCC_APB2Periph_AFIO | RCC_APB2Periph_SPI1, ENABLE);
    GPIO_SetBits(GPIOB, GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_10);
    gpio.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_10;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &gpio);

    gpio.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
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
    spi.SPI_CPHA = SPI_CPHA_2Edge; /* ADS1118: data latched on falling edge */
    spi.SPI_NSS = SPI_NSS_Soft;
    spi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_64; /* 1.125 MHz, extra margin */
    spi.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_Init(SPI1, &spi);
    SPI_Cmd(SPI1, ENABLE);
}

/* ADS1118 datasheet 9.5.7:
 * First 16 clocks write cfg, last 16 clocks return cfg readback.
 * Return CS high exactly after 32 clocks; then wait for the new conversion
 * and read 16 clocks without updating cfg (NOP=00). */
static uint8_t adc420_read_configured(uint8_t chip, uint16_t cfg, int16_t *raw)
{
    uint16_t discarded, echo, data;
    uint32_t started;
    if (raw == 0 || chip > 2U) return 0U;

    adc420_diag_config_written[chip] = cfg;
    adc420_select(chip, Bit_RESET);
    if (!adc420_spi_word(cfg, &discarded) ||
        !adc420_spi_word(0x0000U, &echo) ||
        !adc420_spi_idle()) {
        adc420_select(chip, Bit_SET);
        adc420_diag_spi_state[chip] = 2U;
        return 0U;
    }
    adc420_select(chip, Bit_SET);
    adc420_diag_config_echo[chip] = echo;
    if ((echo & ADC420_VERIFY_MASK) != (cfg & ADC420_VERIFY_MASK)) {
        adc420_diag_spi_state[chip] = 3U;
        return 0U;
    }

    started = ADC420_Millis();
    while ((uint32_t)(ADC420_Millis() - started) < 3U) { }

    adc420_select(chip, Bit_RESET);
    if (!adc420_spi_word(0x0000U, &data) || !adc420_spi_idle()) {
        adc420_select(chip, Bit_SET);
        adc420_diag_spi_state[chip] = 4U;
        return 0U;
    }
    adc420_select(chip, Bit_SET);
    *raw = (int16_t)data;
    adc420_diag_spi_state[chip] = 1U;
    adc420_diag_last_raw[chip] = *raw;
    adc420_diag_read_count[chip]++;
    return 1U;
}

uint8_t ADC420_ADS1118_Read(uint8_t chip, uint8_t ain, int16_t *raw)
{
    uint16_t cfg;
    if (chip > ADC420_CHIP_O3 || ain > 3U || raw == 0) return 0U;
    cfg = (uint16_t)(ADC420_CONFIG_BASE | ((uint16_t)(ain + 4U) << 12));
    return adc420_read_configured(chip, cfg, raw);
}

uint8_t ADC420_ADS1118_SelfTest(void)
{
    uint8_t chip, mask = 0U;
    int16_t temp;
    /* ADS1118 temperature result is a signed 14-bit code left aligned.
     * 25 deg C -> approx 0x0C80 for the 16-bit SPI output (800 * 4).
     * Readback of configuration is the primary digital-link check. */
    for (chip = 0U; chip < 3U; ++chip) {
        if (adc420_read_configured(chip, ADC420_CONFIG_TEMP, &temp)) {
            adc420_diag_temp_raw[chip] = temp;
            /* Temperature plausibility -40..+125 deg C (raw / 128). */
            if (temp > -5120 && temp < 16000) mask |= (uint8_t)(1U << chip);
        } else {
            adc420_diag_temp_raw[chip] = (int16_t)-32768;
        }
    }
    adc420_diag_temp_ok_mask = mask;
    return mask;
}
