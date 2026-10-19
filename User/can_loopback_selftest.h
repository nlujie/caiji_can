#ifndef __CAN_LOOPBACK_SELFTEST_H
#define __CAN_LOOPBACK_SELFTEST_H

/*
 * Diagnostic-only firmware path for STM32F103C8 CAN1.
 * Include from User/main.c only (functions and watch variables are defined here).
 * PA11 = CAN_RX; PA12 = CAN_TX. Internal bxCAN loopback DOES NOT test
 * the external isolated transceiver, choke, TVS or CANH/CANL connector.
 *
 * Keil Watch expressions:
 *   can_selftest_status       0=not started, 1=running, 2=PASS,
 *                             3=clock error, 4=CAN init failed,
 *                             5=no TX mailbox, 6=TX error/timeout,
 *                             7=RX timeout, 8=RX data mismatch
 *   can_selftest_pass, can_selftest_fail, can_selftest_last_seq
 *   can_selftest_last_esr, can_selftest_last_tsr
 */
#include "stm32f10x.h"
#include "stm32f10x_can.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "serial.h"

#define CAN_SELFTEST_FRAME_COUNT   100U
#define CAN_SELFTEST_BITRATE       500000UL
#define CAN_SELFTEST_TQ_PER_BIT    18UL
#define CAN_SELFTEST_POLL_LIMIT    2000000UL

volatile uint32_t can_selftest_status = 0;
volatile uint32_t can_selftest_pass = 0;
volatile uint32_t can_selftest_fail = 0;
volatile uint32_t can_selftest_last_seq = 0;
volatile uint32_t can_selftest_last_id = 0;
volatile uint32_t can_selftest_last_esr = 0;
volatile uint32_t can_selftest_last_tsr = 0;
volatile uint32_t can_selftest_pclk1 = 0;
volatile uint32_t can_selftest_prescaler = 0;
volatile uint32_t can_selftest_tx_status = 0;
volatile uint32_t can_selftest_rx_id = 0;
volatile uint32_t can_selftest_rx_dlc = 0;
volatile uint32_t can_selftest_expected_byte0 = 0;
volatile uint32_t can_selftest_received_byte0 = 0;

static void CAN_SelfTest_PrintU32(uint32_t n)
{
    char digits[11];
    uint8_t index = 0;
    do {
        digits[index++] = (char)('0' + n % 10U);
        n /= 10U;
    } while (n != 0U);
    while (index != 0U) {
        Serial_SendByte((uint8_t)digits[--index]);
    }
}

static void CAN_SelfTest_PrintResult(void)
{
    Serial_SendString("CAN_SELFTEST status=");
    CAN_SelfTest_PrintU32(can_selftest_status);
    Serial_SendString(" pass=");
    CAN_SelfTest_PrintU32(can_selftest_pass);
    Serial_SendString(" fail=");
    CAN_SelfTest_PrintU32(can_selftest_fail);
    Serial_SendString(" seq=");
    CAN_SelfTest_PrintU32(can_selftest_last_seq);
    Serial_SendString("\r\n");
}

static void CAN_SelfTest_Error(uint32_t status)
{
    can_selftest_status = status;
    can_selftest_fail++;
    can_selftest_last_esr = CAN1->ESR;
    can_selftest_last_tsr = CAN1->TSR;
    Serial_SendString("CAN_SELFTEST FAIL\r\n");
    CAN_SelfTest_PrintResult();
}

static void CAN_SelfTest_Run(void)
{
    GPIO_InitTypeDef gpio;
    RCC_ClocksTypeDef clocks;
    CAN_InitTypeDef can_init;
    CAN_FilterInitTypeDef filter;
    CanTxMsg tx;
    CanRxMsg rx;
    uint32_t seq;
    uint32_t wait;
    uint32_t divisor;
    uint8_t mailbox;
    uint8_t tx_status;
    uint8_t i;
    uint8_t matched;

    can_selftest_status = 1U;
    can_selftest_pass = 0U;
    can_selftest_fail = 0U;

    /* USART1: PA9 TX, 9600 8N1 in the existing Serial_Init(). */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
    Serial_Init();
    Serial_SendString("\r\nCAN1 internal loopback selftest (PA11/PA12)\r\n");

    RCC_GetClocksFreq(&clocks);
    can_selftest_pclk1 = clocks.PCLK1_Frequency;
    divisor = CAN_SELFTEST_BITRATE * CAN_SELFTEST_TQ_PER_BIT;
    if ((clocks.PCLK1_Frequency == 0U) ||
        ((clocks.PCLK1_Frequency % divisor) != 0U) ||
        ((clocks.PCLK1_Frequency / divisor) == 0U) ||
        ((clocks.PCLK1_Frequency / divisor) > 1024U)) {
        CAN_SelfTest_Error(3U);
        return;
    }
    can_selftest_prescaler = clocks.PCLK1_Frequency / divisor;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);

    /* Keep the board schematic's default CAN1 pin assignment. */
    gpio.GPIO_Pin = GPIO_Pin_11;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &gpio);
    gpio.GPIO_Pin = GPIO_Pin_12;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &gpio);

    CAN_DeInit(CAN1);
    CAN_StructInit(&can_init);
    can_init.CAN_TTCM = DISABLE;
    can_init.CAN_ABOM = DISABLE;
    can_init.CAN_AWUM = DISABLE;
    can_init.CAN_NART = ENABLE; /* One attempt per frame; fail fast. */
    can_init.CAN_RFLM = DISABLE;
    can_init.CAN_TXFP = DISABLE;
    can_init.CAN_Mode = CAN_Mode_LoopBack; /* SILM=0, LBKM=1 */
    can_init.CAN_SJW = CAN_SJW_1tq;
    can_init.CAN_BS1 = CAN_BS1_15tq;
    can_init.CAN_BS2 = CAN_BS2_2tq;
    can_init.CAN_Prescaler = (uint16_t)can_selftest_prescaler;
    if (CAN_Init(CAN1, &can_init) == CANINITFAILED) {
        CAN_SelfTest_Error(4U);
        return;
    }

    /* 32-bit mask = 0: accept all IDs, FIFO0, polling (no RX IRQ). */
    filter.CAN_FilterNumber = 0;
    filter.CAN_FilterMode = CAN_FilterMode_IdMask;
    filter.CAN_FilterScale = CAN_FilterScale_32bit;
    filter.CAN_FilterIdHigh = 0U;
    filter.CAN_FilterIdLow = 0U;
    filter.CAN_FilterMaskIdHigh = 0U;
    filter.CAN_FilterMaskIdLow = 0U;
    filter.CAN_FilterFIFOAssignment = CAN_Filter_FIFO0;
    filter.CAN_FilterActivation = ENABLE;
    CAN_FilterInit(&filter);

    CAN_ITConfig(CAN1, CAN_IT_FMP0, DISABLE);
    NVIC_DisableIRQ(USB_LP_CAN1_RX0_IRQn);
    Serial_SendString("500kbps (from actual PCLK1), 100 frames\r\n");

    for (seq = 0U; seq < CAN_SELFTEST_FRAME_COUNT; seq++) {
        can_selftest_last_seq = seq;
        tx.StdId = 0x321U + (seq & 0x0FU);
        tx.ExtId = 0U;
        tx.IDE = CAN_Id_Standard;
        tx.RTR = CAN_RTR_Data;
        tx.DLC = (uint8_t)(seq % 9U); /* Exercise DLC 0..8. */
        for (i = 0U; i < 8U; i++) {
            tx.Data[i] = (uint8_t)(0xA5U ^ (uint8_t)seq ^ (uint8_t)(i * 17U));
        }
        can_selftest_last_id = tx.StdId;
        can_selftest_expected_byte0 = tx.Data[0];

        mailbox = CAN_Transmit(CAN1, &tx);
        if (mailbox == CAN_TxStatus_NoMailBox) {
            CAN_SelfTest_Error(5U);
            return;
        }

        tx_status = CAN_TxStatus_Pending;
        wait = CAN_SELFTEST_POLL_LIMIT;
        while (wait-- != 0U) {
            tx_status = CAN_TransmitStatus(CAN1, mailbox);
            if (tx_status != CAN_TxStatus_Pending) {
                break;
            }
        }
        can_selftest_tx_status = tx_status;
        if (tx_status != CAN_TxStatus_Ok) {
            CAN_SelfTest_Error(6U);
            return;
        }

        wait = CAN_SELFTEST_POLL_LIMIT;
        while ((CAN_MessagePending(CAN1, CAN_FIFO0) == 0U) && (wait-- != 0U)) {
            /* Poll the internal CAN RX FIFO. */
        }
        if (CAN_MessagePending(CAN1, CAN_FIFO0) == 0U) {
            CAN_SelfTest_Error(7U);
            return;
        }
        CAN_Receive(CAN1, CAN_FIFO0, &rx);

        can_selftest_rx_id = rx.StdId;
        can_selftest_rx_dlc = rx.DLC;
        can_selftest_received_byte0 = rx.Data[0];
        matched = (uint8_t)((rx.IDE == CAN_Id_Standard) &&
                            (rx.RTR == CAN_RTR_Data) &&
                            (rx.StdId == tx.StdId) && (rx.DLC == tx.DLC));
        if (matched != 0U) {
            for (i = 0U; i < tx.DLC; i++) {
                if (rx.Data[i] != tx.Data[i]) {
                    matched = 0U;
                    break;
                }
            }
        }
        if (matched == 0U) {
            CAN_SelfTest_Error(8U);
            return;
        }

        can_selftest_pass++;
        if ((can_selftest_pass % 10U) == 0U) {
            CAN_SelfTest_PrintResult();
        }
    }

    can_selftest_last_esr = CAN1->ESR;
    can_selftest_last_tsr = CAN1->TSR;
    can_selftest_status = 2U;
    Serial_SendString("CAN_SELFTEST PASS: 100/100 frames match\r\n");
    CAN_SelfTest_PrintResult();
}

#endif /* __CAN_LOOPBACK_SELFTEST_H */
