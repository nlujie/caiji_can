#include "adc420_can.h"
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_can.h"

extern uint32_t ADC420_Millis(void);

uint8_t ADC420_CAN_Init(void)
{
    GPIO_InitTypeDef gpio;
    CAN_InitTypeDef can;
    CAN_FilterInitTypeDef filter;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);
    /* CAN1 default pins PA11/PA12 (do NOT remap to PB8/PB9). */
    GPIO_PinRemapConfig(GPIO_Remap1_CAN1, DISABLE);

    gpio.GPIO_Pin = GPIO_Pin_11;
    gpio.GPIO_Mode = GPIO_Mode_IPU;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);
    gpio.GPIO_Pin = GPIO_Pin_12;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &gpio);

    CAN_DeInit(CAN1);
    CAN_StructInit(&can);
    can.CAN_TTCM = DISABLE;
    can.CAN_ABOM = ENABLE;
    can.CAN_AWUM = ENABLE;
    can.CAN_NART = DISABLE;   /* permit retransmission */
    can.CAN_RFLM = DISABLE;
    can.CAN_TXFP = DISABLE;
    can.CAN_Mode = CAN_Mode_Normal;
    can.CAN_SJW = CAN_SJW_1tq;
    can.CAN_BS1 = CAN_BS1_9tq;
    can.CAN_BS2 = CAN_BS2_8tq;
    can.CAN_Prescaler = 4U;  /* 36MHz/(4*(1+9+8)) = 500kbps */
    if (CAN_Init(CAN1, &can) == CANINITFAILED) return 0U;

    filter.CAN_FilterNumber = 0U;
    filter.CAN_FilterMode = CAN_FilterMode_IdMask;
    filter.CAN_FilterScale = CAN_FilterScale_32bit;
    filter.CAN_FilterIdHigh = 0U;
    filter.CAN_FilterIdLow = 0U;
    filter.CAN_FilterMaskIdHigh = 0U;
    filter.CAN_FilterMaskIdLow = 0U;
    filter.CAN_FilterFIFOAssignment = CAN_Filter_FIFO0;
    filter.CAN_FilterActivation = ENABLE;
    CAN_FilterInit(&filter);
    return 1U;
}

uint8_t ADC420_CAN_Send(uint16_t std_id, const uint8_t *bytes, uint8_t count)
{
    CanTxMsg tx;
    uint8_t mailbox, i;
    uint32_t since;
    uint32_t limit = 100000UL;

    if (bytes == 0 || count > 8U || std_id > 0x7FFU) return 0U;
    tx.StdId = std_id;
    tx.ExtId = 0U;
    tx.IDE = CAN_Id_Standard;
    tx.RTR = CAN_RTR_Data;
    tx.DLC = count;
    for (i = 0U; i < 8U; ++i) tx.Data[i] = (i < count) ? bytes[i] : 0U;

    mailbox = CAN_Transmit(CAN1, &tx);
    if (mailbox == CAN_TxStatus_NoMailBox) return 0U;
    since = ADC420_Millis();
    while (((uint32_t)(ADC420_Millis() - since) < 4U) && (limit-- > 0U)) {
        uint8_t state = CAN_TransmitStatus(CAN1, mailbox);
        if (state == CAN_TxStatus_Ok) return 1U;
        if (state == CAN_TxStatus_Failed) break;
    }
    CAN_CancelTransmit(CAN1, mailbox);
    return 0U;
}
