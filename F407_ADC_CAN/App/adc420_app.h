#ifndef F407_ADC420_APP_H
#define F407_ADC420_APP_H

#include <stdint.h>

#define ADC420_COUNT       10U
#define ADC420_PERIOD_MS   100UL
#define ADC420_CAN_ID_0    0x310U
#define ADC420_CAN_ID_1    0x311U
#define ADC420_CAN_ID_2    0x312U
#define ADC420_CAN_ID_STATUS 0x313U

typedef struct {
    uint8_t board_channel; /* logical ADC_2 ... ADC_11 */
    uint8_t chip;          /* 0=N_O1, 1=N_O2, 2=N_O3 */
    uint8_t ain;           /* ADS1118 AIN0 ... AIN3 */
} ADC420_Mapping;

extern const ADC420_Mapping adc420_map[ADC420_COUNT];
extern volatile int16_t adc420_raw[ADC420_COUNT];
extern volatile uint16_t adc420_current_ua[ADC420_COUNT];
extern volatile uint16_t adc420_valid_mask;
extern volatile uint16_t adc420_fault_mask;
extern volatile uint8_t adc420_sequence;
extern volatile uint32_t adc420_scan_count;
extern volatile uint8_t adc420_last_can_failures;

/* Dedicated runtime diagnostics for Keil Watch window. */
extern volatile uint8_t adc420_debug_init_stage;
extern volatile uint32_t adc420_debug_system_clock_hz;
extern volatile uint8_t adc420_debug_spi_init_ok;
extern volatile uint8_t adc420_debug_can_init_ok;
extern volatile uint32_t adc420_debug_main_loop_count;
extern volatile uint8_t adc420_debug_scan_in_progress;
extern volatile uint8_t adc420_debug_current_index;
extern volatile uint8_t adc420_debug_current_board_channel;
extern volatile uint8_t adc420_debug_current_chip;
extern volatile uint8_t adc420_debug_current_ain;
extern volatile uint8_t adc420_debug_last_spi_ok;
extern volatile uint8_t adc420_debug_last_spi_state;
extern volatile int16_t adc420_debug_last_raw;
extern volatile uint16_t adc420_debug_last_current_ua;
extern volatile uint32_t adc420_debug_scan_duration_ms;
extern volatile uint32_t adc420_debug_channel_success_count[ADC420_COUNT];
extern volatile uint32_t adc420_debug_channel_fail_count[ADC420_COUNT];

uint16_t ADC420_Convert_uA(int16_t raw);
void ADC420_App_ScanAndSend(void);

#endif
