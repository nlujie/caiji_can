#include "adc420_spi.h"
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_spi.h"

/*
 * Original PCB schematic:
 * N_O1: PB0 -> ISO7741F -> /CS; MCU PB0 LOW selects chip 1.
 * N_O2: PB1 -> optocoupler -> /CS; MCU PB1 HIGH selects chip 2.
 * N_O3: PB10 -> optocoupler -> /CS; MCU PB10 HIGH selects chip 3.
 * SPI1 PA5=SCK PA6=MISO PA7=MOSI shared between all 3 chips.
 *
 * TI ADS1118: SPI mode 1, 16-bit conversion + optional 16-bit
 * configuration echo, in total 32 clocks. Single shot, 860SPS,
 * PGA +/-2.048V, AINx-GND mux, internal reference.
 */
#define ADC420_SPI_TIMEOUT_LOOPS 40000UL
#define ADC420_CONFIG_BASE 0x85EBU
#define ADC420_ECHO_MASK 0x7FFEU /* SS bit15 always reads 0; bit0 reserved */
extern volatile uint32_t adc420_ms;

volatile uint8_t adc420_spi_state[3];
volatile uint16_t adc420_spi_cfg_sent[3];
volatile uint16_t adc420_spi_cfg_echo[3];
volatile uint16_t adc420_spi_previous_result[3];
volatile uint32_t adc420_spi_success_count[3];

static void adc420_spin_us(uint32_t us)
{
    /* Conservative CS settling delay for optocouplers (not a calibrated timer). */
    volatile uint32_t n;
    uint32_t ticks = (SystemCoreClock / 6000000UL) * us;
    for (n = 0UL; n < ticks; ++n) { }
}
static void adc420_wait_ms(uint32_t ms)
{
    uint32_t before = adc420_ms;
    while ((uint32_t)(adc420_ms - before) < ms) { }
}
static void adc420_deselect_all(void)
{
    GPIO_SetBits(GPIOB, GPIO_Pin_0);
    GPIO_ResetBits(GPIOB, GPIO_Pin_1 | GPIO_Pin_10);
}
static void adc420_select(uint8_t chip)
{
    adc420_deselect_all();
    if (chip == ADC420_CHIP_O1) GPIO_ResetBits(GPIOB, GPIO_Pin_0);
    else if (chip == ADC420_CHIP_O2) GPIO_SetBits(GPIOB, GPIO_Pin_1);
    else GPIO_SetBits(GPIOB, GPIO_Pin_10);
    adc420_spin_us(100UL); /* optical /CS settling */
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
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA |
                           RCC_APB2Periph_GPIOB |
                           RCC_APB2Periph_AFIO |
                           RCC_APB2Periph_SPI1, ENABLE);

    /* Preload the output levels BEFORE making these GPIOs outputs. */
    adc420_deselect_all();
    gpio.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_10;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
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
    spi.SPI_CPHA = SPI_CPHA_2Edge;
    spi.SPI_NSS = SPI_NSS_Soft;
    spi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_64;
    spi.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_Init(SPI1, &spi);
    SPI_Cmd(SPI1, ENABLE);
    adc420_spin_us(100UL);
}
uint8_t ADC420_SPI_Read(uint8_t chip, uint8_t channel, int16_t *result)
{
    uint16_t discard, cfg, echo, data;
    uint8_t ok;
    if (result == 0 || chip > 2U || channel > 3U) return 0U;
    /* 0xC5EB,0xD5EB,0xE5EB,0xF5EB for AIN0..3 to GND. */
    cfg = (uint16_t)(ADC420_CONFIG_BASE | ((uint16_t)(channel + 4U) << 12));
    adc420_spi_cfg_sent[chip] = cfg;

    /* First transfer clocks old data out; starts one NEW conversion.
       The second 16 clocks return CONFIG echo. */
    adc420_select(chip);
    ok = (uint8_t)(adc420_xfer16(cfg, &discard) &&
                   adc420_xfer16(0x0000U, &echo) &&
                   adc420_spi_done());
    adc420_deselect();
    if (!ok) {
        adc420_spi_state[chip] = 2U;
        return 0U;
    }
    adc420_spi_cfg_echo[chip] = echo;
    adc420_spi_previous_result[chip] = discard;
    if (((uint16_t)(echo ^ cfg) & ADC420_ECHO_MASK) != 0U) {
        adc420_spi_state[chip] = 3U;
        return 0U;
    }

    /* Fresh conversion for newly selected AIN. Wait > 1/860 s. */
    adc420_wait_ms(3UL);
    adc420_select(chip);
    ok = (uint8_t)(adc420_xfer16(0x0000U, &data) && adc420_spi_done());
    adc420_deselect();
    if (!ok) {
        adc420_spi_state[chip] = 2U;
        return 0U;
    }
    *result = (int16_t)data;
    adc420_spi_state[chip] = 1U;
    adc420_spi_success_count[chip]++;
    return 1U;
}
