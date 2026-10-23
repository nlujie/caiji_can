#ifndef ADC420_SPI_H
#define ADC420_SPI_H

#include <stdint.h>

#define ADC420_CHIP_O1 0U
#define ADC420_CHIP_O2 1U
#define ADC420_CHIP_O3 2U

/* SPI/ADS1118 diagnostics for Keil Watch.
   state: 0=not read yet, 1=read OK, 2=SPI timeout, 3=config echo mismatch. */
extern volatile uint8_t adc420_spi_state[3];
extern volatile uint16_t adc420_spi_cfg_sent[3];
extern volatile uint16_t adc420_spi_cfg_echo[3];
extern volatile uint16_t adc420_spi_previous_result[3];
extern volatile uint32_t adc420_spi_success_count[3];

extern volatile uint8_t adc420_debug_spi_init_done;
extern volatile uint8_t adc420_debug_spi_current_chip;
extern volatile uint8_t adc420_debug_spi_current_channel;
extern volatile uint8_t adc420_debug_spi_last_error;
extern volatile uint32_t adc420_debug_cs_gpio_odr;

void ADC420_SPI_Init(void);
uint8_t ADC420_SPI_Read(uint8_t chip, uint8_t channel, int16_t *result);

#endif
