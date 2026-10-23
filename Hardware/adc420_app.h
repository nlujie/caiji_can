#ifndef __ADC420_APP_H
#define __ADC420_APP_H

#include "stm32f10x.h"

#define ADC420_CHANNEL_COUNT  10U
#define ADC420_CAN_FRAME_COUNT 5U
#define ADC420_CAN_ID_BASE    0x301U

/*
 * Channel array index is ADC ID - 2:
 * index 0..9 correspond to ADC_2..ADC_11.
 *
 * CAN payload: each channel uses signed int16 raw counts and signed int16
 * current in 0.01 mA units, both little-endian.
 */
extern volatile int16_t adc420_raw[ADC420_CHANNEL_COUNT];
extern volatile float   adc420_voltage_v[ADC420_CHANNEL_COUNT];
extern volatile float   adc420_current_ma[ADC420_CHANNEL_COUNT];
extern volatile int16_t adc420_current_centi_ma[ADC420_CHANNEL_COUNT];
extern volatile uint8_t adc420_channel_valid[ADC420_CHANNEL_COUNT];

extern volatile uint8_t  adc420_scan_ok;
extern volatile uint32_t adc420_scan_count;
extern volatile uint32_t adc420_scan_fail_count;
extern volatile uint32_t adc420_can_frame_count;
extern volatile uint32_t adc420_can_error_count;
extern volatile uint16_t adc420_last_can_id;
extern volatile uint8_t  adc420_last_can_status;
extern volatile uint8_t  adc420_last_adc_id;
extern volatile uint8_t  adc420_last_chip;
extern volatile uint8_t  adc420_last_channel;

void ADC420_Init(void);
uint8_t ADC420_ScanAll(void);
uint8_t ADC420_SendCanFrames(void);

#endif
