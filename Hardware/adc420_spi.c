#include "adc420_spi.h"
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_spi.h"

/* STM32F103C8 + three ADS1118:
   SPI1: PA5=SCK, PA6=MISO, PA7=MOSI.
   CS: PB0=N_O1, PB1=N_O2, PB10=N_O3.
   All three chip selects are active LOW: LOW selects, HIGH deselects. */
#define ADC420_SPI_TIMEOUT_LOOPS 40000UL
#define ADC420_CONFIG_BASE       0x85EBU
#define ADC420_ECHO_MASK         0x7FFEU

extern volatile uint32_t adc420_ms;

volatile uint8_t adc420_spi_state[3] = {0U, 0U, 0U};
volatile uint16_t adc420_spi_cfg_sent[3] = {0U, 0U, 0U};
volatile uint16_t adc420_spi_cfg_echo[3] = {0U, 0U, 0U};
volatile uint16_t adc420_spi_previous_result[3] = {0U, 0U, 0U};
volatile uint32_t adc420_spi_success_count[3] = {0UL, 0UL, 0UL};

volatile uint8_t adc420_debug_spi_init_done = 0U;
volatile uint8_t adc420_debug_spi_current_chip = 0xFFU;
volatile uint8_t adc420_debug_spi_current_channel = 0xFFU;
volatile uint8_t adc420_debug_spi_last_error = 0U;
volatile uint32_t adc420_debug_cs_gpio_odr = 0UL;

static void adc420_spin_us(uint32_t us)
{
    volatile uint32_t n;
    uint32_t loops = (SystemCoreClock / 6000000UL) * us;
    for (n = 0UL; n < loops; ++n) {
        /* Short settling delay for the CS isolation path. */
    }
}

static void adc420_wait_ms(uint32_t ms)
{
    uint32_t before = adc420_ms;
    while ((uint32_t)(adc420_ms - before) < ms) {
        /* SysTick must be running. */
    }
}

static void adc420_deselect_all(void)
{
    /* Active-low CS: HIGH on all three pins means all chips deselected. */
    GPIO_SetBits(GPIOB, GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_10);
    adc420_debug_cs_gpio_odr = GPIOB->ODR;
}

static void adc420_select(uint8_t chip)
{
    adc420_deselect_all();
    if (chip == ADC420_CHIP_O1) {
        GPIO_ResetBits(GPIOB, GPIO_Pin_0);
    } else if (chip == ADC420_CHIP_O2) {
        GPIO_ResetBits(GPIOB, GPIO_Pin_1);
    } else {
        GPIO_ResetBits(GPIOB, GPIO_Pin_10);
    }
    adc420_debug_cs_gpio_odr = GPIOB->ODR;
    adc420_spin_us(100UL);
}

static void adc420_deselect(void)
{
    adc420_spin_us(10UL);
    adc420_deselect_all();
    adc420_spin_us(100UL);
}

static uint8_t adc420_xfer8(uint8_t value, uint8_t *out)
{
    uint32_t timeout;
    if (out == 0) return 0U;

    for (timeout = ADC420_SPI_TIMEOUT_LOOPS; timeout > 0UL; --timeout) {
        if (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) != RESET) break;
    }
    if (timeout == 0UL) return 0U;

    SPI_I2S_SendData(SPI1, value);

    for (timeout = ADC420_SPI_TIMEOUT_LOOPS; timeout > 0UL; --timeout) {
        if (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) != RESET) break;
    }
    if (timeout == 0UL) return 0U;

    *out = (uint8_t)SPI_I2S_ReceiveData(SPI1);
    return 1U;
}

static uint8_t adc420_xfer16(uint16_t value, uint16_t *out)
{
    uint8_t msb, lsb;
    if (out == 0) return 0U;
    if (!adc420_xfer8((uint8_t)(value >> 8), &msb)) return 0U;
    if (!adc420_xfer8((uint8_t)value, &lsb)) return 0U;
    *out = (uint16_t)(((uint16_t)msb << 8) | lsb);
    return 1U;
}

static uint8_t adc420_spi_done(void)
{
    uint32_t timeout;
    for (timeout = ADC420_SPI_TIMEOUT_LOOPS; timeout > 0UL; --timeout) {
        if (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == RESET) return 1U;
    }
    return 0U;
}

void ADC420_SPI_Init(void)
{
    GPIO_InitTypeDef gpio;
    SPI_InitTypeDef spi;

    adc420_debug_spi_init_done = 0U;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA |
                           RCC_APB2Periph_GPIOB |
                           RCC_APB2Periph_AFIO |
                           RCC_APB2Periph_SPI1, ENABLE);

    /* Set inactive HIGH levels before enabling CS pins as outputs. */
    GPIO_SetBits(GPIOB, GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_10);
    gpio.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_10;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOB, &gpio);
    adc420_deselect_all();

    /* SPI1 pins: PA5 SCK and PA7 MOSI are AF push-pull; PA6 is MISO input. */
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
    spi.SPI_CPHA = SPI_CPHA_2Edge; /* ADS1118 SPI mode 1 */
    spi.SPI_NSS = SPI_NSS_Soft;
    spi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_64;
    spi.SPI_FirstBit = SPI_FirstBit_MSB;
    spi.SPI_CRCPolynomial = 7U;
    SPI_Init(SPI1, &spi);
    SPI_Cmd(SPI1, ENABLE);
    adc420_spin_us(100UL);

    adc420_debug_cs_gpio_odr = GPIOB->ODR;
    adc420_debug_spi_init_done = 1U;
}

uint8_t ADC420_SPI_Read(uint8_t chip, uint8_t channel, int16_t *result)
{
    uint16_t discarded = 0U, cfg, echo = 0U, data = 0U;
    uint8_t ok;

    adc420_debug_spi_current_chip = chip;
    adc420_debug_spi_current_channel = channel;
    adc420_debug_spi_last_error = 0U;

    if (result == 0 || chip > ADC420_CHIP_O3 || channel > 3U) {
        adc420_debug_spi_last_error = 1U;
        return 0U;
    }

    /* AINx-to-GND, PGA +/-2.048 V, single-shot, 860 SPS.
       4-20 mA through 100 ohms becomes 0.4-2.0 V. */
    cfg = (uint16_t)(ADC420_CONFIG_BASE | ((uint16_t)(channel + 4U) << 12));
    adc420_spi_cfg_sent[chip] = cfg;

    /* First 32 clocks read the previous result and write the new config. */
    adc420_select(chip);
    ok = (uint8_t)(adc420_xfer16(cfg, &discarded) &&
                   adc420_xfer16(0x0000U, &echo) &&
                   adc420_spi_done());
    adc420_deselect();

    if (!ok) {
        adc420_spi_state[chip] = 2U;
        adc420_debug_spi_last_error = 2U;
        return 0U;
    }

    adc420_spi_previous_result[chip] = discarded;
    adc420_spi_cfg_echo[chip] = echo;
    if (((uint16_t)(echo ^ cfg) & ADC420_ECHO_MASK) != 0U) {
        adc420_spi_state[chip] = 3U;
        adc420_debug_spi_last_error = 3U;
        return 0U;
    }

    /* 860 SPS conversion time is about 1.16 ms; allow 3 ms. */
    adc420_wait_ms(3UL);
    adc420_select(chip);
    ok = (uint8_t)(adc420_xfer16(0x0000U, &data) && adc420_spi_done());
    adc420_deselect();

    if (!ok) {
        adc420_spi_state[chip] = 2U;
        adc420_debug_spi_last_error = 2U;
        return 0U;
    }

    *result = (int16_t)data;
    adc420_spi_state[chip] = 1U;
    adc420_spi_success_count[chip]++;
    return 1U;
}
