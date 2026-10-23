#ifndef F407_ADC420_CAN_H
#define F407_ADC420_CAN_H

#include <stdint.h>

extern volatile uint8_t adc420_can_initialized;
extern volatile uint32_t adc420_can_tx_ok;
extern volatile uint32_t adc420_can_tx_failed;

/* Last attempted CAN frame/result and hardware snapshots for Keil Watch.
   last_result: 0=not sent, 1=TX OK, 2=invalid/not initialized,
   3=no free mailbox, 4=timeout/no ACK, 5=controller TX error. */
extern volatile uint16_t adc420_debug_can_last_id;
extern volatile uint8_t adc420_debug_can_last_dlc;
extern volatile uint8_t adc420_debug_can_last_data[8];
extern volatile uint8_t adc420_debug_can_last_result;
extern volatile uint32_t adc420_debug_can_attempt_count;
extern volatile uint32_t adc420_debug_can_esr;
extern volatile uint32_t adc420_debug_can_msr;
extern volatile uint32_t adc420_debug_can_tsr;
extern volatile uint32_t adc420_debug_can_btr;

uint8_t ADC420_CAN_Init(void);
uint8_t ADC420_CAN_Send(uint16_t id, const uint8_t *data, uint8_t len);

#endif
