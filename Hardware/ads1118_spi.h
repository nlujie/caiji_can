#ifndef __ADS1118_SPI_H
#define __ADS1118_SPI_H

#include "stm32f10x.h"

/* All three ADS1118 devices share SPI1. Chip-select inputs are active-low. */
#define ADS1118_CHIP_N_O1  0U  /* PB0 */
#define ADS1118_CHIP_N_O2  1U  /* PB1 */
#define ADS1118_CHIP_N_O3  2U  /* PB10 */

extern volatile uint8_t  ads1118_spi_ready;
extern volatile uint32_t ads1118_spi_timeout_count;
extern volatile uint8_t  ads1118_last_chip;
extern volatile uint8_t  ads1118_last_channel;
extern volatile uint16_t ads1118_last_config;
extern volatile uint16_t ads1118_last_raw;

/* Initializes SPI1 and drives all three active-low chip selects inactive. */
void ADS1118_Init(void);

/*
 * Reads one single-ended ADS1118 input (AIN0..AIN3) from the selected chip.
 * Returns 1 on success and writes the signed 16-bit conversion result to raw.
 */
uint8_t ADS1118_ReadChannel(uint8_t chip, uint8_t channel, int16_t *raw);

#endif
