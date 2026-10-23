#ifndef ADC420_ADS1118_H
#define ADC420_ADS1118_H
#include <stdint.h>

/* Based on board schematic: SPI1, CS N_O1=PB0, N_O2=PB1, N_O3=PB10. */
#define ADC420_CHIP_O1 0U
#define ADC420_CHIP_O2 1U
#define ADC420_CHIP_O3 2U

/* Diagnostic state per ADS1118:
 * 0=not yet tested, 1=verified, 2=SPI transfer timeout,
 * 3=config readback mismatch, 4=result-read timeout. */
extern volatile uint8_t adc420_diag_spi_state[3];
extern volatile uint16_t adc420_diag_config_written[3];
extern volatile uint16_t adc420_diag_config_echo[3];
extern volatile int16_t adc420_diag_last_raw[3];
extern volatile uint32_t adc420_diag_read_count[3];
extern volatile uint8_t adc420_diag_temp_ok_mask;
extern volatile int16_t adc420_diag_temp_raw[3];

void ADC420_ADS1118_Init(void);
uint8_t ADC420_ADS1118_Read(uint8_t chip, uint8_t ain, int16_t *raw);
/* One-off power-on diagnostic via internal temperature sensor, independent
 * of external sensor wiring. Returns bitmask of responding devices. */
uint8_t ADC420_ADS1118_SelfTest(void);

#endif
