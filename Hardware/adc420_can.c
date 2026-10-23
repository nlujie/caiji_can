#include "adc420_can.h"
#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_can.h"

extern volatile uint32_t adc420_ms;
volatile uint8_t adc420_can_initialized;
volatile uint32_t adc420_can_tx_ok;
volatile uint32_t adc420_can_tx_failed;

void ADC420_CAN_Init(void)
{
    GPIO_InitTypeDef gpio;
    CAN_InitTypeDef can;
    CAN_FilterInitTypeDef filter;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);
    /* PA11 RX, PA12 TX; NOT PB8/PB9 (those are PT100 START/DRDY). */
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
    can.CAN_ABOM = ENABLE; /* automatic bus-off recovery */
    can.CAN_AWUM = ENABLE;
    can.CAN_NART = DISABLE; /* auto retransmit */
    can.CAN_RFLM = DISABLE;
    can.CAN_TXFP = DISABLE;
    can.CAN_Mode = CAN_Mode_Normal;
    can.CAN_SJW = CAN_SJW_1tq;
    can.CAN_BS1 = CAN_BS1_9tq;
    can.CAN_BS2 = CAN_BS2_8tq;
    /* APB1=36MHz, 36M/(4*(1+9+8))=500kbps. */
    can.CAN_Prescaler = 4U;
    adc420_can_initialized = (CAN_Init(CAN1, &can) == CANINITFAILED) ? 0U : 1U;

    filter.CAN_FilterNumber = 0U;
    filter.CAN_FilterMode = CAN_FilterMode_IdMask;
    filter.CAN_FilterScale = CAN_FilterScale_32bit;
    filter.CAN_FilterIdHigh = 0U;
    filter.CAN_FilterIdLow = 0U;
    filter.CAN_FilterMaskIdHigh = 0U;
    filter.CAN_FilterMaskIdLow = 0U;
    filter.CAN_FilterFIFOAssignment = CAN_Filter_FIFO0;
    filter.CAN_FilterActivation = ENABLE;
    if (adc420_can_initialized) CAN_FilterInit(&filter);
}

uint8_t ADC420_CAN_Send(uint16_t id, const uint8_t *data, uint8_t len)
{
    CanTxMsg message;
    uint8_t mailbox, i, state;
    uint32_t start;
    if (!adc420_can_initialized || data == 0 || len > 8U || id > 0x7FFU) return 0U;
    message.StdId = id;
    message.ExtId = 0UL;
    message.IDE = CAN_Id_Standard;
    message.RTR = CAN_RTR_Data;
    message.DLC = len;
    for (i = 0U; i < 8U; i++) message.Data[i] = (i < len) ? data[i] : 0U;
    mailbox = CAN_Transmit(CAN1, &message);
    if (mailbox == CAN_TxStatus_NoMailBox) {
        adc420_can_tx_failed++;
        return 0U;
    }
    start = adc420_ms;
    while ((uint32_t)(adc420_ms - start) < 8UL) {
        state = CAN_TransmitStatus(CAN1, mailbox);
        if (state == CAN_TxStatus_Ok) {
            adc420_can_tx_ok++;
            return 1U;
        }
        if (state == CAN_TxStatus_Failed) break;
    }
    CAN_CancelTransmit(CAN1, mailbox);
    adc420_can_tx_failed++;
    return 0U;
}
