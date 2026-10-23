#ifndef __ADC420_APP_H
#define __ADC420_APP_H
#include "stm32f10x.h"

#define ADC420_CHANNEL_COUNT 10U

/* Values are in 0.01mA. Out-of-range/negative readings are clamped to 0. */
extern volatile uint16_t adc420_raw[ADC420_CHANNEL_COUNT];
extern volatile uint16_t adc420_millivolt[ADC420_CHANNEL_COUNT];
extern volatile uint16_t adc420_centi_ma[ADC420_CHANNEL_COUNT];

void ADC420_Init(void);
void ADC420_ScanAll(uint16_t current_centi_ma[ADC420_CHANNEL_COUNT]);

#endif
