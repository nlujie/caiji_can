#include "can.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_can.h"

#define CAN_TX_TIMEOUT  200000UL

volatile uint8_t  can_init_ok = 0U;
volatile uint8_t  can_last_tx_ok = 0U;
volatile uint32_t can_tx_count = 0U;
volatile uint32_t can_tx_fail_count = 0U;

uint8_t CAN_Module_Init(void)
{
    GPIO_InitTypeDef gpio;
    CAN_InitTypeDef can;
    CAN_FilterInitTypeDef filter;

    can_init_ok = 0U;
    can_last_tx_ok = 0U;
    can_tx_count = 0U;
    can_tx_fail_count = 0U;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);

    /* Use the default CAN1 mapping: PA12=TX, PA11=RX. */
    GPIO_PinRemapConfig(GPIO_Remap1_CAN1, DISABLE);

    gpio.GPIO_Pin = GPIO_Pin_12;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &gpio);

    gpio.GPIO_Pin = GPIO_Pin_11;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &gpio);

    CAN_DeInit(CAN1);

    can.CAN_TTCM = DISABLE;
    can.CAN_ABOM = ENABLE;
    can.CAN_AWUM = ENABLE;
    can.CAN_NART = DISABLE;  /* Automatic retransmission enabled. */
    can.CAN_RFLM = DISABLE;
    can.CAN_TXFP = DISABLE;
    can.CAN_Mode = CAN_Mode_Normal;
    can.CAN_SJW = CAN_SJW_1tq;
    can.CAN_BS1 = CAN_BS1_9tq;
    can.CAN_BS2 = CAN_BS2_8tq;
    can.CAN_Prescaler = 4U;

    /*
     * 36 MHz / (4 * (1 + 9 + 8)) = 500 kbit/s.
     * This assumes the usual 72 MHz SYSCLK and 36 MHz APB1 clock.
     */
    if (CAN_Init(CAN1, &can) != CANINITOK)
    {
        return 0U;
    }

    /* Accept all standard/extended IDs into FIFO0; this application only TXes. */
    filter.CAN_FilterNumber = 0U;
    filter.CAN_FilterMode = CAN_FilterMode_IdMask;
    filter.CAN_FilterScale = CAN_FilterScale_32bit;
    filter.CAN_FilterIdHigh = 0x0000U;
    filter.CAN_FilterIdLow = 0x0000U;
    filter.CAN_FilterMaskIdHigh = 0x0000U;
    filter.CAN_FilterMaskIdLow = 0x0000U;
    filter.CAN_FilterFIFOAssignment = CAN_Filter_FIFO0;
    filter.CAN_FilterActivation = ENABLE;
    CAN_FilterInit(&filter);

    can_init_ok = 1U;
    return 1U;
}

uint8_t CAN_SendMessage(CanTxMsg *TxMessage)
{
    uint8_t mailbox;
    uint8_t status;
    uint32_t timeout;

    can_last_tx_ok = 0U;

    if ((can_init_ok == 0U) || (TxMessage == 0) || (TxMessage->DLC > 8U))
    {
        can_tx_fail_count++;
        return CAN_TxStatus_Failed;
    }

    mailbox = CAN_Transmit(CAN1, TxMessage);
    if (mailbox == CAN_TxStatus_NoMailBox)
    {
        can_tx_fail_count++;
        return CAN_TxStatus_NoMailBox;
    }

    timeout = CAN_TX_TIMEOUT;
    while (timeout-- > 0U)
    {
        status = CAN_TransmitStatus(CAN1, mailbox);

        if (status == CAN_TxStatus_Ok)
        {
            can_last_tx_ok = 1U;
            can_tx_count++;
            return CAN_TxStatus_Ok;
        }

        if (status == CAN_TxStatus_Failed)
        {
            break;
        }
    }

    CAN_CancelTransmit(CAN1, mailbox);
    can_tx_fail_count++;
    return CAN_TxStatus_Failed;
}
