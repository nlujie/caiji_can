#ifndef ADC420_ADS1118_H
#define ADC420_ADS1118_H

#include <stdint.h>

/* All three ADS1118s are connected to SPI1.
 * N_O1 = PB0, N_O2 = PB1, N_O3 = PB10 (per board schematic). */
#define ADC420_CHIP_O1 0U
#define ADC420_CHIP_O2 1U
#define ADC420_CHIP_O3 2U

void ADC420_ADS1118_Init(void);
/* Returns 1 on successful SPI transaction; raw is signed 16-bit ADC code.
 * Uses a fresh, single-shot conversion per channel (not prior MUX data). */
uint8_t ADC420_ADS1118_Read(uint8_t chip, uint8_t ain, int16_t *raw);

#endif
