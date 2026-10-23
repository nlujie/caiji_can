#ifndef ADC420_CAN_H
#define ADC420_CAN_H
#include <stdint.h>
extern volatile uint8_t adc420_can_initialized;
extern volatile uint32_t adc420_can_tx_ok;
extern volatile uint32_t adc420_can_tx_failed;
void ADC420_CAN_Init(void);
uint8_t ADC420_CAN_Send(uint16_t id, const uint8_t *data, uint8_t len);
#endif
