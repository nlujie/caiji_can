#ifndef F407_ADC_CAN_TIMEBASE_H
#define F407_ADC_CAN_TIMEBASE_H

#include <stdint.h>

extern volatile uint32_t adc420_ms;
void Timebase_Init(void);
uint32_t Timebase_Millis(void);
void Delay_ms(uint32_t ms);

#endif
