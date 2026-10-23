#ifndef __ADC420_APP_H
#define __ADC420_APP_H

#include "stm32f10x.h"

#define ADC420_CHANNEL_COUNT  10U
#define ADC420_CAN_FRAME_COUNT 3U
#define ADC420_CAN_ID_BASE    0x110U

/*
 * ADC array index is ADC ID - 2:
 * index 0..9 correspond to ADC_2..ADC_11.
 *
 * CAN transmits raw ADS1118 conversion results only. Each ADC result is a
 * signed 16-bit value encoded little-endian. No current/engineering value is
 * included in the CAN payload.
 *
 * Frame 0x110: ADC_10, ADC_9, ADC_8, ADC_11 (DLC=8)
 * Frame 0x111: ADC_5, ADC_4, ADC_7, ADC_6 (DLC=8)
 * Frame 0x112: ADC_3, ADC_2 (DLC=4)
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
void ADC420_DebugPrintReadings(void);
uint8_t ADC420_SendCanFrames(void);

#endif
