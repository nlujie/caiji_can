#include "adc420_can.h"
#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_can.h"

extern volatile uint32_t adc420_ms;

volatile uint8_t adc420_can_initialized = 0U;
volatile uint32_t adc420_can_tx_ok = 0UL;
volatile uint32_t adc420_can_tx_failed = 0UL;

volatile uint16_t adc420_debug_can_last_id = 0U;
volatile uint8_t adc420_debug_can_last_dlc = 0U;
volatile uint8_t adc420_debug_can_last_data[8] = {0U,0U,0U,0U,0U,0U,0U,0U};
volatile uint8_t adc420_debug_can_last_result = 0U;
/* last_result: 0=idle, 1=success, 2=invalid/uninitialized,
   3=no mailbox, 4=timeout, 5=transmit failed. */
volatile uint32_t adc420_debug_can_attempt_count = 0UL;
volatile uint32_t adc420_debug_can_esr = 0UL;
volatile uint32_t adc420_debug_can_msr = 0UL;
volatile uint32_t adc420_debug_can_tsr = 0UL;
volatile uint32_t adc420_debug_can_btr = 0UL;

static void adc420_debug_can_snapshot(void)
{
    adc420_debug_can_esr = CAN1->ESR;
    adc420_debug_can_msr = CAN1->MSR;
    adc420_debug_can_tsr = CAN1->TSR;
    adc420_debug_can_btr = CAN1->BTR;
}

uint8_t ADC420_CAN_Init(void)
{
    GPIO_InitTypeDef gpio;
    CAN_InitTypeDef can;
    CAN_FilterInitTypeDef filter;

    adc420_can_initialized = 0U;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);

    /* CAN1 default pins: PA11=RX, PA12=TX. */
    GPIO_PinRemapConfig(GPIO_Remap1_CAN1, DISABLE);
    gpio.GPIO_Pin = GPIO_Pin_11;
    gpio.GPIO_Mode = GPIO_Mode_IPU;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    gpio.GPIO_Pin = GPIO_Pin_12;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    CAN_DeInit(CAN1);
    CAN_StructInit(&can);
    can.CAN_TTCM = DISABLE;
    can.CAN_ABOM = ENABLE;
    can.CAN_AWUM = ENABLE;
    can.CAN_NART = DISABLE; /* auto retransmit */
    can.CAN_RFLM = DISABLE;
    can.CAN_TXFP = DISABLE;
    can.CAN_Mode = CAN_Mode_Normal;
    can.CAN_SJW = CAN_SJW_1tq;
    can.CAN_BS1 = CAN_BS1_9tq;
    can.CAN_BS2 = CAN_BS2_8tq;
    /* APB1=36 MHz: 36 MHz / (4 * (1+9+8)) = 500 kbit/s. */
    can.CAN_Prescaler = 4U;

    if (CAN_Init(CAN1, &can) == CANINITFAILED) {
        adc420_debug_can_snapshot();
        return 0U;
    }

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

    adc420_can_initialized = 1U;
    adc420_debug_can_snapshot();
    return 1U;
}

uint8_t ADC420_CAN_Send(uint16_t id, const uint8_t *data, uint8_t len)
{
    CanTxMsg message;
    uint8_t mailbox, i, state;
    uint32_t start;

    adc420_debug_can_attempt_count++;
    adc420_debug_can_last_id = id;
    adc420_debug_can_last_dlc = len;
    adc420_debug_can_last_result = 0U;
    for (i = 0U; i < 8U; i++) {
        adc420_debug_can_last_data[i] =
            (data != 0 && i < len && len <= 8U) ? data[i] : 0U;
    }

    if (!adc420_can_initialized || data == 0 || len > 8U || id > 0x7FFU) {
        adc420_can_tx_failed++;
        adc420_debug_can_last_result = 2U;
        adc420_debug_can_snapshot();
        return 0U;
    }

    message.StdId = id;
    message.ExtId = 0UL;
    message.IDE = CAN_Id_Standard;
    message.RTR = CAN_RTR_Data;
    message.DLC = len;
    for (i = 0U; i < 8U; i++) {
        message.Data[i] = (i < len) ? data[i] : 0U;
    }

    mailbox = CAN_Transmit(CAN1, &message);
    if (mailbox == CAN_TxStatus_NoMailBox) {
        adc420_can_tx_failed++;
        adc420_debug_can_last_result = 3U;
        adc420_debug_can_snapshot();
        return 0U;
    }

    start = adc420_ms;
    while ((uint32_t)(adc420_ms - start) < 8UL) {
        state = CAN_TransmitStatus(CAN1, mailbox);
        if (state == CAN_TxStatus_Ok) {
            adc420_can_tx_ok++;
            adc420_debug_can_last_result = 1U;
            adc420_debug_can_snapshot();
            return 1U;
        }
        if (state == CAN_TxStatus_Failed) break;
    }

    CAN_CancelTransmit(CAN1, mailbox);
    adc420_can_tx_failed++;
    adc420_debug_can_last_result = 4U;
    adc420_debug_can_snapshot();
    return 0U;
}
