#include "adc420_app.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_spi.h"

#define ADC420_CS_O1_LOW()  GPIO_ResetBits(GPIOB, GPIO_Pin_0)
#define ADC420_CS_O1_HIGH() GPIO_SetBits(GPIOB, GPIO_Pin_0)
#define ADC420_CS_O2_LOW()  GPIO_ResetBits(GPIOB, GPIO_Pin_1)
#define ADC420_CS_O2_HIGH() GPIO_SetBits(GPIOB, GPIO_Pin_1)
#define ADC420_CS_O3_LOW()  GPIO_ResetBits(GPIOB, GPIO_Pin_10)
#define ADC420_CS_O3_HIGH() GPIO_SetBits(GPIOB, GPIO_Pin_10)

volatile uint16_t adc420_raw[ADC420_CHANNEL_COUNT];
volatile uint16_t adc420_millivolt[ADC420_CHANNEL_COUNT];
volatile uint16_t adc420_centi_ma[ADC420_CHANNEL_COUNT];

typedef enum { ADC420_O1 = 0, ADC420_O2, ADC420_O3 } ADC420_Chip;

static void wait_cycles(volatile uint32_t n)
{
    while (n--) { __NOP(); }
}
static void wait_ms(uint32_t ms)
{
    while (ms--) wait_cycles(SystemCoreClock / 2000U);
}
static void all_cs_high(void)
{
    ADC420_CS_O1_HIGH();
    ADC420_CS_O2_HIGH();
    ADC420_CS_O3_HIGH();
}
static void select_chip(ADC420_Chip chip)
{
    all_cs_high();
    /* ADS1118 /CS is active-low. Only one chip is selected at a time. */
    if (chip == ADC420_O1) ADC420_CS_O1_LOW();
    else if (chip == ADC420_O2) ADC420_CS_O2_LOW();
    else ADC420_CS_O3_LOW();
}
static uint8_t spi_transfer(SPI_TypeDef *spi, uint8_t data)
{
    uint32_t timeout = 100000U;
    while (SPI_I2S_GetFlagStatus(spi, SPI_I2S_FLAG_TXE) == RESET && timeout--) { }
    if (timeout == 0U) return 0U;
    SPI_I2S_SendData(spi, data);
    timeout = 100000U;
    while (SPI_I2S_GetFlagStatus(spi, SPI_I2S_FLAG_RXNE) == RESET && timeout--) { }
    if (timeout == 0U) return 0U;
    return (uint8_t)SPI_I2S_ReceiveData(spi);
}
static uint16_t ads1118_read(ADC420_Chip chip, uint8_t channel)
{
    uint16_t config;
    uint8_t hi, lo;
    int16_t signed_raw;
    SPI_TypeDef *spi = (chip == ADC420_O3) ? SPI2 : SPI1;

    /* OS=1 single-shot; MUX=AINx-to-GND; PGA=+/-2.048V;
       DR=128SPS. Config base 0xC4EB matches the existing board driver. */
    config = (uint16_t)(0xC58BU | ((uint16_t)(channel & 0x03U) << 12));
    select_chip(chip);
    wait_cycles(80U);
    (void)spi_transfer(spi, (uint8_t)(config >> 8));
    (void)spi_transfer(spi, (uint8_t)config);
    all_cs_high();

    /* 128SPS conversion needs up to 7.8ms; leave margin. */
    wait_ms(10U);

    select_chip(chip);
    wait_cycles(80U);
    hi = spi1_transfer((uint8_t)(config >> 8));
    lo = spi1_transfer((uint8_t)config);
    all_cs_high();

    signed_raw = (int16_t)(((uint16_t)hi << 8) | lo);
    if (signed_raw < 0) return 0U;
    return (uint16_t)signed_raw;
}
static void store_channel(uint8_t logical_index, ADC420_Chip chip, uint8_t mux)
{
    uint16_t raw = ads1118_read(chip, mux);
    uint32_t mv;
    uint32_t centi_ma;

    /* ADS1118 +/-2.048V: 62.5uV/LSB. Rounded millivolts. */
    mv = ((uint32_t)raw * 625U + 5000U) / 10000U;
    /* External 100-ohm shunt: 1mA -> 100mV, so mA=millivolts/100.
       Convert to 0.01mA units: mV/1 (numerically equal to centi-mA). */
    centi_ma = mv;
    if (centi_ma > 2500U) centi_ma = 2500U;
    adc420_raw[logical_index] = raw;
    adc420_millivolt[logical_index] = (uint16_t)mv;
    adc420_centi_ma[logical_index] = (uint16_t)centi_ma;
}

void ADC420_Init(void)
{
    GPIO_InitTypeDef gpio;
    SPI_InitTypeDef spi;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |
                           RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);

    /* SPI1: PA5=SCK, PA6=MISO, PA7=MOSI. */
    gpio.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &gpio);
    gpio.GPIO_Pin = GPIO_Pin_6;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio);

    /* Three board chip selects: N_O1=PB0 (SPI1), N_O2=PB1 (SPI1),
       N_O3=PB10 (SPI2). All are active-low; only one chip selected at a time. */
    gpio.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_10;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &gpio);
    all_cs_high();

    SPI_StructInit(&spi);
    spi.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    spi.SPI_Mode = SPI_Mode_Master;
    spi.SPI_DataSize = SPI_DataSize_8b;
    spi.SPI_CPOL = SPI_CPOL_Low;
    spi.SPI_CPHA = SPI_CPHA_2Edge;
    spi.SPI_NSS = SPI_NSS_Soft;
    spi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;
    spi.SPI_FirstBit = SPI_FirstBit_MSB;
    spi.SPI_CRCPolynomial = 7;
    SPI_Init(SPI1, &spi);
    SPI_Cmd(SPI1, ENABLE);

    /* N_O3 ADC is wired to SPI2: PB13=SCK, PB14=MISO, PB15=MOSI. */
    gpio.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_15;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOB, &gpio);
    gpio.GPIO_Pin = GPIO_Pin_14;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOB, &gpio);
    SPI_Init(SPI2, &spi);
    SPI_Cmd(SPI2, ENABLE);
}

void ADC420_ScanAll(uint16_t current_centi_ma[ADC420_CHANNEL_COUNT])
{
    uint8_t i;
    /* Logical order ADC_2..ADC_11; wiring follows page 1-3 of usage guide. */
    store_channel(0U, ADC420_O2, 2U);  /* ADC_2: N_O2 AIN2 */
    store_channel(1U, ADC420_O2, 3U);  /* ADC_3: N_O2 AIN3 */
    store_channel(2U, ADC420_O1, 0U);  /* ADC_4: N_O1 AIN0 */
    store_channel(3U, ADC420_O1, 1U);  /* ADC_5: N_O1 AIN1 */
    store_channel(4U, ADC420_O1, 3U);  /* ADC_6: N_O1 AIN3 */
    store_channel(5U, ADC420_O1, 2U);  /* ADC_7: N_O1 AIN2 */
    store_channel(6U, ADC420_O3, 2U);  /* ADC_8: N_O3 AIN2 */
    store_channel(7U, ADC420_O3, 1U);  /* ADC_9: N_O3 AIN1 */
    store_channel(8U, ADC420_O3, 3U);  /* ADC_10: N_O3 AIN3 */
    store_channel(9U, ADC420_O3, 0U);  /* ADC_11: N_O3 AIN0 */
    for (i = 0U; i < ADC420_CHANNEL_COUNT; ++i)
        current_centi_ma[i] = adc420_centi_ma[i];
}
