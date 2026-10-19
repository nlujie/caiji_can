#ifndef __CAN_UPLINK_H
#define __CAN_UPLINK_H

/* STM32F103C8 / bxCAN CAN1, PA11=RX, PA12=TX.
 * CAN2.0A standard data frames, 500 kbit/s, 8 bytes, 200 ms sample interval.
 * Kept compatible with original main_can_v0_1019_1504 frame IDs.
 *
 * 0x110  ADS1118 on SPI1 chip1: AIN0, AIN1, AIN2, AIN3.
 * 0x111  ADS1118 on SPI1 chip2: AIN0, AIN1, AIN2, AIN3.
 * 0x112  ADS1118 on SPI2 chip1: AIN0, AIN1, AIN2, AIN3.
 *
 * All payloads are 4 unsigned, filtered ADC codes (uint16_t),
 * little-endian: Data[0]=AIN0 low, [1]=AIN0 high, ..., [7]=AIN3 high.
 * No floating point, no scaling, no sequence field. Controller must agree.
 */
#include "stm32f10x.h"
#include "func.h"

#define CAN_UPLINK_BITRATE_BPS           500000UL
#define CAN_UPLINK_TX_TIMEOUT_MS         50UL
#define CAN_UPLINK_ID_SPI1_CHIP1         0x110U
#define CAN_UPLINK_ID_SPI1_CHIP2         0x111U
#define CAN_UPLINK_ID_SPI2_CHIP1         0x112U

/* Debug/watch values:
 * init_status: 0 = not initialized; 1 = normal CAN ready;
 *              2 = wrong PCLK1 for exact bitrate; 3 = CAN_Init failure.
 * tx_ok counts confirmed CAN transmission completion (ACK received).
 * batch_ok counts complete three-frame sensor snapshots sent.
 * tx_fail / tx_timeout count delivery attempts that failed or timed out.
 * Note: CAN ACK only proves one bus participant acknowledged the frame;
 * it does not prove the application controller consumed sensor data.
 */
extern volatile uint32_t can_uplink_init_status;
extern volatile uint32_t can_uplink_pclk1_hz;
extern volatile uint32_t can_uplink_prescaler;
extern volatile uint32_t can_uplink_tx_ok;
extern volatile uint32_t can_uplink_tx_fail;
extern volatile uint32_t can_uplink_tx_timeout;
extern volatile uint32_t can_uplink_batch_ok;
extern volatile uint32_t can_uplink_pending_overrun;
extern volatile uint32_t can_uplink_last_id;
extern volatile uint32_t can_uplink_last_esr;
extern volatile uint32_t can_uplink_last_tsr;
extern volatile uint32_t can_uplink_rx_count;
extern volatile uint32_t can_uplink_last_tx_status;

uint8_t CAN_Uplink_Init(void);
void CAN_Uplink_Publish(const ADC_Raw_Data_t *chip1,
                        const ADC_Raw_Data_t *chip2,
                        const ADC_Raw_Data_t *chip3);
void CAN_Uplink_Poll(uint32_t now_ms);

#endif
