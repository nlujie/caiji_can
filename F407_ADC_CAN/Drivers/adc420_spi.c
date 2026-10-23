#include "adc420_spi.h"
#include "stm32f407_min.h"
#include "timebase.h"

/*
 * Board GPIO chip-select polarity confirmed by user (all active LOW):
 * N_O1: PB0 LOW selects (through ISO7741).
 * N_O2: PB1 LOW selects (through the optocoupler path).
 * N_O3: PB10 LOW selects (through the optocoupler path).
 * SPI1: PA5=SCK, PA6=MISO, PA7=MOSI; SPI mode 1.
 */
#define ADC420_SPI_TIMEOUT_LOOPS 40000UL
#define ADC420_CONFIG_BASE       0x85EBU
#define ADC420_ECHO_MASK         0x7FFEU

volatile uint8_t adc420_spi_state[3] = {0U, 0U, 0U};
volatile uint16_t adc420_spi_cfg_sent[3] = {0U, 0U, 0U};
volatile uint16_t adc420_spi_cfg_echo[3] = {0U, 0U, 0U};
volatile uint16_t adc420_spi_previous_result[3] = {0U, 0U, 0U};
volatile uint32_t adc420_spi_success_count[3] = {0UL, 0UL, 0UL};

static void adc420_spin_us(uint32_t us)
{
    volatile uint32_t n;
    uint32_t loops = (SystemCoreClock / 3000000UL) * us;
    for (n = 0UL; n < loops; ++n) {
        /* volatile loop: CS settling for isolator/optocoupler */
    }
}

static void adc420_deselect_all(void)
{
    /* PB0=1, PB1=1, PB10=1 means all three ADS1118 chips deselected. */
    GPIOB_BSRR = (1UL << 0) | (1UL << 1) | (1UL << 10);
}

static void adc420_select(uint8_t chip)
{
    adc420_deselect_all();
    if (chip == ADC420_CHIP_O1) {
        GPIOB_BSRR = 1UL << (0U + 16U); /* PB0 LOW */
    } else if (chip == ADC420_CHIP_O2) {
        GPIOB_BSRR = 1UL << (1U + 16U); /* PB1 LOW */
    } else {
        GPIOB_BSRR = 1UL << (10U + 16U); /* PB10 LOW */
    }
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
        if ((SPI1_SR & (1UL << 1)) != 0UL) break; /* TXE */
    }
    if (timeout == 0UL) return 0U;

    *(volatile uint8_t *)(uintptr_t)(SPI1_BASE + 0x0CUL) = value;

    for (timeout = ADC420_SPI_TIMEOUT_LOOPS; timeout > 0UL; --timeout) {
        if ((SPI1_SR & (1UL << 0)) != 0UL) break; /* RXNE */
    }
    if (timeout == 0UL) return 0U;

    *out = *(volatile uint8_t *)(uintptr_t)(SPI1_BASE + 0x0CUL);
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
        if ((SPI1_SR & (1UL << 7)) == 0UL) return 1U; /* BSY clear */
    }
    return 0U;
}

void ADC420_SPI_Init(void)
{
    uint32_t pinmask;

    RCC_AHB1ENR |= (1UL << 0) | (1UL << 1); /* GPIOA/GPIOB */
    RCC_APB2ENR |= (1UL << 12);             /* SPI1 */
    (void)RCC_APB2ENR;

    adc420_deselect_all();

    pinmask = (3UL << 0) | (3UL << 2) | (3UL << 20);
    GPIOB_MODER = (GPIOB_MODER & ~pinmask) |
                  (1UL << 0) | (1UL << 2) | (1UL << 20);
    GPIOB_OTYPER &= ~((1UL << 0) | (1UL << 1) | (1UL << 10));
    GPIOB_OSPEEDR = (GPIOB_OSPEEDR & ~pinmask) |
                    (3UL << 0) | (3UL << 2) | (3UL << 20);
    GPIOB_PUPDR &= ~pinmask;

    pinmask = (3UL << 10) | (3UL << 12) | (3UL << 14);
    GPIOA_MODER = (GPIOA_MODER & ~pinmask) |
                  (2UL << 10) | (2UL << 12) | (2UL << 14);
    GPIOA_OTYPER &= ~((1UL << 5) | (1UL << 6) | (1UL << 7));
    GPIOA_OSPEEDR = (GPIOA_OSPEEDR & ~pinmask) |
                    (3UL << 10) | (3UL << 12) | (3UL << 14);
    GPIOA_PUPDR &= ~pinmask;
    GPIOA_AFRL = (GPIOA_AFRL & ~((15UL << 20) | (15UL << 24) | (15UL << 28))) |
                 (5UL << 20) | (5UL << 24) | (5UL << 28); /* AF5: SPI1 */

    SPI1_CR1 = 0UL;
    SPI1_CR2 = 0UL;
    SPI1_CR1 = (1UL << 0) | (1UL << 2) | (6UL << 3) |
               (1UL << 8) | (1UL << 9); /* CPHA=1, master, /64, SSM+SSI */
    SPI1_CR1 |= (1UL << 6); /* SPE */
    adc420_spin_us(100UL);
}

uint8_t ADC420_SPI_Read(uint8_t chip, uint8_t channel, int16_t *result)
{
    uint16_t discarded = 0U, cfg, echo = 0U, data = 0U;
    uint8_t ok;

    if (result == 0 || chip > ADC420_CHIP_O3 || channel > 3U) return 0U;

    /* AINx-to-GND mux; PGA +/-2.048 V; single-shot; 860 SPS.
       4-20 mA through 100 ohms gives 0.400-2.000 V (about 6400-32000 counts). */
    cfg = (uint16_t)(ADC420_CONFIG_BASE | ((uint16_t)(channel + 4U) << 12));
    adc420_spi_cfg_sent[chip] = cfg;

    /* 32-clock ADS1118 frame: first 16 clocks read the prior conversion and
       write config; second 16 clocks return the configuration-register echo. */
    adc420_select(chip);
    ok = (uint8_t)(adc420_xfer16(cfg, &discarded) &&
                   adc420_xfer16(0x0000U, &echo) &&
                   adc420_spi_done());
    adc420_deselect();
    if (!ok) {
        adc420_spi_state[chip] = 2U;
        return 0U;
    }

    adc420_spi_previous_result[chip] = discarded;
    adc420_spi_cfg_echo[chip] = echo;
    if (((uint16_t)(echo ^ cfg) & ADC420_ECHO_MASK) != 0U) {
        adc420_spi_state[chip] = 3U;
        return 0U;
    }

    /* 860 SPS conversion is about 1.16 ms; 3 ms gives margin. */
    Delay_ms(3UL);
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
