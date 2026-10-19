#ifndef __CAN_UPLINK_H
#define __CAN_UPLINK_H

/* ADC telemetry uplink for STM32F103C8 bxCAN1.
   Header is intentionally instantiated only from User/main.c.
   Protocol: standard 11-bit IDs 0x110/0x111/0x112; DLC=8;
   AIN0..AIN3 unsigned 16-bit, little-endian, filtered raw ADC counts.
   These provisional IDs are inherited from the original project.
   Do not infer engineering units from the raw ADC payload.
*/
#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_can.h"
#include "can.h"
#include "func.h"

#define CAN_UPLINK_BITRATE           500000UL
#define CAN_UPLINK_TQ_PER_BIT        18UL
#define CAN_UPLINK_TX_TIMEOUT_MS     30UL

volatile uint32_t can_uplink_init_ok = 0;
volatile uint32_t can_uplink_pclk1 = 0;
volatile uint32_t can_uplink_prescaler = 0;
volatile uint32_t can_uplink_tx_ok = 0;
volatile uint32_t can_uplink_tx_fail = 0;
volatile uint32_t can_uplink_tx_timeout = 0;
volatile uint32_t can_uplink_mailbox_busy = 0;
volatile uint32_t can_uplink_frames_queued = 0;
volatile uint32_t can_uplink_rx_count = 0;
volatile uint32_t can_uplink_last_id = 0;
volatile uint32_t can_uplink_last_esr = 0;
volatile uint32_t can_uplink_last_tsr = 0;

static uint8_t can_uplink_payload[3][8];
static uint8_t can_uplink_next = 0;
static uint8_t can_uplink_active = 0;
static uint8_t can_uplink_mailbox = 0;
static uint32_t can_uplink_start_ms = 0;
static const uint16_t can_uplink_ids[3] = {
    CAN_ID_SPI1_CHIP1_RAW, CAN_ID_SPI1_CHIP2_RAW, CAN_ID_SPI2_RAW
};

static void CAN_Uplink_EncodeAdc(uint8_t *out, const ADC_Raw_Data_t *data)
{
    const uint16_t v[4] = {data->ain0, data->ain1, data->ain2, data->ain3};
    uint8_t i;
    for (i = 0; i < 4; i++) {
        out[i * 2] = (uint8_t)(v[i] & 0xFFU);
        out[i * 2 + 1] = (uint8_t)(v[i] >> 8);
    }
}

static void CAN_Uplink_Init(void)
{
    RCC_ClocksTypeDef clocks;
    uint32_t divisor;
    CAN_InitTypeDef config;
    CAN_FilterInitTypeDef filter;
    GPIO_InitTypeDef gpio;

    RCC_GetClocksFreq(&clocks);
    can_uplink_pclk1 = clocks.PCLK1_Frequency;
    divisor = CAN_UPLINK_BITRATE * CAN_UPLINK_TQ_PER_BIT;
    if ((clocks.PCLK1_Frequency == 0U) ||
        ((clocks.PCLK1_Frequency % divisor) != 0U) ||
        (clocks.PCLK1_Frequency / divisor < 1U) ||
        (clocks.PCLK1_Frequency / divisor > 1024U)) {
        can_uplink_init_ok = 0U;
        return;
    }
    can_uplink_prescaler = clocks.PCLK1_Frequency / divisor;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);
    GPIO_PinRemapConfig(GPIO_Remap1_CAN1, DISABLE);
    GPIO_PinRemapConfig(GPIO_Remap2_CAN1, DISABLE);

    gpio.GPIO_Pin = GPIO_Pin_11; /* CAN RX */
    gpio.GPIO_Mode = GPIO_Mode_IPU;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);
    gpio.GPIO_Pin = GPIO_Pin_12; /* CAN TX */
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &gpio);

    CAN_DeInit(CAN1);
    CAN_StructInit(&config);
    config.CAN_TTCM = DISABLE;
    config.CAN_ABOM = ENABLE;    /* recover from bus-off */
    config.CAN_AWUM = DISABLE;
    config.CAN_NART = DISABLE;   /* standard automatic retransmission */
    config.CAN_RFLM = DISABLE;
    config.CAN_TXFP = DISABLE;
    config.CAN_Mode = CAN_Mode_Normal;
    config.CAN_SJW = CAN_SJW_1tq;
    config.CAN_BS1 = CAN_BS1_15tq;
    config.CAN_BS2 = CAN_BS2_2tq;
    config.CAN_Prescaler = (uint16_t)can_uplink_prescaler;
    if (CAN_Init(CAN1, &config) == CANINITFAILED) {
        can_uplink_init_ok = 0U;
        return;
    }

    filter.CAN_FilterNumber = 0;
    filter.CAN_FilterMode = CAN_FilterMode_IdMask;
    filter.CAN_FilterScale = CAN_FilterScale_32bit;
    filter.CAN_FilterIdHigh = 0;
    filter.CAN_FilterIdLow = 0;
    filter.CAN_FilterMaskIdHigh = 0;
    filter.CAN_FilterMaskIdLow = 0;
    filter.CAN_FilterFIFOAssignment = CAN_Filter_FIFO0;
    filter.CAN_FilterActivation = ENABLE;
    CAN_FilterInit(&filter);

    /* Preserve original received-message callback and FIFO0 IRQ. */
    CAN_SetRxCallback(CAN_Rx_Callback);
    CAN_EnableInterrupt();
    can_uplink_init_ok = 1U;
}

static void CAN_Uplink_QueueLatest(void)
{
    if (!can_uplink_init_ok) return;
    /* Do not overwrite an in-flight three-frame snapshot. */
    if (can_uplink_active) return;
    CAN_Uplink_EncodeAdc(can_uplink_payload[0], &raw_adc_spi1_chip1);
    CAN_Uplink_EncodeAdc(can_uplink_payload[1], &raw_adc_spi1_chip2);
    CAN_Uplink_EncodeAdc(can_uplink_payload[2], &raw_adc_spi2);
    can_uplink_next = 0U;
    can_uplink_mailbox = CAN_TxStatus_NoMailBox;
    can_uplink_active = 1U;
    can_uplink_frames_queued += 3U;
}

/* Nonblocking service: one frame at a time; abort if the bus has no ACK.
   A TX_OK means the CAN controller reported successful transmission/ACK,
   NOT that the receiving application's parser processed the payload.
*/
static void CAN_Uplink_Service(uint32_t now_ms)
{
    uint8_t status;
    uint8_t i;
    CanTxMsg frame;

    if (!can_uplink_init_ok || !can_uplink_active) return;

    can_uplink_last_esr = CAN1->ESR;
    can_uplink_last_tsr = CAN1->TSR;

    if (can_uplink_mailbox != CAN_TxStatus_NoMailBox) {
        status = CAN_TransmitStatus(CAN1, can_uplink_mailbox);
        if (status == CAN_TxStatus_Pending) {
            if ((uint32_t)(now_ms - can_uplink_start_ms) <= CAN_UPLINK_TX_TIMEOUT_MS) return;
            CAN_CancelTransmit(CAN1, can_uplink_mailbox);
            can_uplink_tx_timeout++;
            can_uplink_tx_fail++;
        } else if (status == CAN_TxStatus_Ok) {
            can_uplink_tx_ok++;
        } else {
            can_uplink_tx_fail++;
        }
        can_uplink_mailbox = CAN_TxStatus_NoMailBox;
        can_uplink_next++;
        if (can_uplink_next >= 3U) {
            can_uplink_active = 0U;
            return;
        }
    }

    frame.StdId = can_uplink_ids[can_uplink_next];
    frame.ExtId = 0U;
    frame.IDE = CAN_Id_Standard;
    frame.RTR = CAN_RTR_Data;
    frame.DLC = 8U;
    for (i = 0U; i < 8U; i++) frame.Data[i] = can_uplink_payload[can_uplink_next][i];

    can_uplink_mailbox = CAN_Transmit(CAN1, &frame);
    if (can_uplink_mailbox == CAN_TxStatus_NoMailBox) {
        can_uplink_mailbox_busy++;
        /* Try again on the next service iteration. */
        return;
    }
    can_uplink_last_id = frame.StdId;
    can_uplink_start_ms = now_ms;
}

#endif
