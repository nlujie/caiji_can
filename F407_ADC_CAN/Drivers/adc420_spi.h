#ifndef F407_ADC420_SPI_H
#define F407_ADC420_SPI_H

#include <stdint.h>

#define ADC420_CHIP_O1 0U
#define ADC420_CHIP_O2 1U
#define ADC420_CHIP_O3 2U

/* Runtime diagnostics for Keil Watch: 0=not run, 1=OK, 2=SPI timeout,
   3=ADS1118 configuration echo mismatch. */
extern volatile uint8_t adc420_spi_state[3];
extern volatile uint16_t adc420_spi_cfg_sent[3];
extern volatile uint16_t adc420_spi_cfg_echo[3];
extern volatile uint16_t adc420_spi_previous_result[3];
extern volatile uint32_t adc420_spi_success_count[3];

/* Live SPI/CS diagnostics. chip: 0=N_O1, 1=N_O2, 2=N_O3.
   last_error: 0=OK/none, 1=invalid argument, 2=SPI timeout,
   3=configuration echo mismatch. */
extern volatile uint8_t adc420_debug_spi_init_done;
extern volatile uint8_t adc420_debug_spi_current_chip;
extern volatile uint8_t adc420_debug_spi_current_channel;
extern volatile uint8_t adc420_debug_spi_last_error;
extern volatile uint32_t adc420_debug_cs_gpio_odr;

void ADC420_SPI_Init(void);
uint8_t ADC420_SPI_Read(uint8_t chip, uint8_t channel, int16_t *result);

#endif
