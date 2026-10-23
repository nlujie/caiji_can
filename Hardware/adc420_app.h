#ifndef __ADC420_APP_H
#define __ADC420_APP_H

#include "stm32f10x.h"

#define ADC420_CHANNEL_COUNT       10U
#define ADC420_TEMP_COUNT          4U
#define ADC420_PRESSURE_COUNT      4U
#define ADC420_FLOW_COUNT          2U
#define ADC420_CAN_FRAME_COUNT     3U
#define ADC420_CAN_ID_BASE         0x110U

/*
 * ADC array index is ADC ID - 2:
 * adc420_raw[0] = ADC_2, ... adc420_raw[9] = ADC_11.
 *
 * Engineering values use fixed-point representation:
 * temperature: signed int16, 0.01 degree C per count
 * pressure: unsigned int16, 0.01 bar per count
 * flow: signed int16 raw ADS1118 conversion result
 */
extern volatile int16_t adc420_raw[ADC420_CHANNEL_COUNT];
extern volatile float   adc420_voltage_v[ADC420_CHANNEL_COUNT];
extern volatile float   adc420_current_ma[ADC420_CHANNEL_COUNT];
extern volatile int16_t adc420_current_centi_ma[ADC420_CHANNEL_COUNT];
extern volatile uint8_t adc420_channel_valid[ADC420_CHANNEL_COUNT];

extern volatile int16_t  adc420_temperature_centi_c[ADC420_TEMP_COUNT];
extern volatile uint16_t adc420_pressure_centi_bar[ADC420_PRESSURE_COUNT];

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
