#ifndef __CAN_H
#define __CAN_H

#include "stm32f10x.h"
#include "stm32f10x_can.h"

extern volatile uint8_t  can_init_ok;
extern volatile uint8_t  can_last_tx_ok;
extern volatile uint32_t can_tx_count;
extern volatile uint32_t can_tx_fail_count;

/* CAN1 on PA11/PA12, normal mode, 500 kbit/s for PCLK1 = 36 MHz. */
uint8_t CAN_Module_Init(void);

/* Returns CAN_TxStatus_Ok on success, otherwise a CAN transmit status. */
uint8_t CAN_SendMessage(CanTxMsg *TxMessage);

#endif
