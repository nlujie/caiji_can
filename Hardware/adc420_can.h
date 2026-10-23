#ifndef ADC420_CAN_H
#define ADC420_CAN_H

#include <stdint.h>

/* Dedicated 500kbps CAN1 interface on PA11 RX / PA12 TX. */
uint8_t ADC420_CAN_Init(void);
/* Return 1 only after bxCAN reports a transmitted/ACKed frame. */
uint8_t ADC420_CAN_Send(uint16_t std_id, const uint8_t *bytes, uint8_t count);

#endif
