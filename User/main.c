#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_can.h"
#include "adc420_app.h"

/* Debug counters: add these to Keil Watch while debugging. */
volatile uint32_t can_tx_success_count = 0U;
volatile uint32_t can_tx_failed_count = 0U;
volatile uint32_t can_tx_timeout_count = 0U;
volatile uint32_t can_tx_no_mailbox_count = 0U;
volatile uint8_t  can_tx_last_status = CAN_TxStatus_Pending;

static void ClockDelay(volatile uint32_t n)
{
    while (n--) { __NOP(); }
}

static void DelayMs(uint32_t ms)
{
    while (ms--) ClockDelay(SystemCoreClock / 2000U);
}

static uint8_t CAN1_Init_500K(void)
{
    GPIO_InitTypeDef gpio;
    CAN_InitTypeDef can;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);

    /* Default CAN1 pins: PA12=TX, PA11=RX. */
    gpio.GPIO_Pin = GPIO_Pin_12;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &gpio);

    gpio.GPIO_Pin = GPIO_Pin_11;
    gpio.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &gpio);

    CAN_DeInit(CAN1);
    CAN_StructInit(&can);
    can.CAN_TTCM = DISABLE;
    can.CAN_ABOM = ENABLE;
    can.CAN_AWUM = ENABLE;
    can.CAN_NART = DISABLE; /* automatic retransmission enabled */
    can.CAN_RFLM = DISABLE;
    can.CAN_TXFP = ENABLE;
    can.CAN_Mode = CAN_Mode_Normal;
    can.CAN_SJW = CAN_SJW_1tq;
    can.CAN_BS1 = CAN_BS1_9tq;
    can.CAN_BS2 = CAN_BS2_8tq;
    /* System clock=72MHz, APB1=36MHz: 36MHz/(4*(1+9+8))=500kbps. */
    can.CAN_Prescaler = 4U;

    if (CAN_Init(CAN1, &can) == CAN_InitStatus_Failed) {
        return 0U;
    }

    /*
     * This application only transmits. Do not enable RX FIFO interrupts
     * without an RX ISR; otherwise an incoming frame can enter Default_Handler.
     * No receive filter setup is needed for CAN transmission.
     */
    return 1U;
}

static void CAN1_SendFrame(CanTxMsg *tx)
{
    uint8_t mailbox;
    uint8_t status = CAN_TxStatus_Pending;
    uint32_t timeout = 100000U;

    mailbox = CAN_Transmit(CAN1, tx);
    if (mailbox == CAN_TxStatus_NoMailBox) {
        can_tx_no_mailbox_count++;
        can_tx_last_status = CAN_TxStatus_NoMailBox;
        return;
    }

    while (timeout > 0U) {
        status = CAN_TransmitStatus(CAN1, mailbox);
        if ((status == CAN_TxStatus_Ok) ||
            (status == CAN_TxStatus_Failed)) {
            break;
        }
        timeout--;
    }

    /* Recheck after the wait to avoid misclassifying a just-completed frame. */
    if (status == CAN_TxStatus_Pending) {
        status = CAN_TransmitStatus(CAN1, mailbox);
    }

    can_tx_last_status = status;
    if (status == CAN_TxStatus_Ok) {
        can_tx_success_count++;
    } else if (status == CAN_TxStatus_Failed) {
        can_tx_failed_count++;
    } else {
        /* Prevent a bus-offline/pending frame from occupying a mailbox forever. */
        can_tx_timeout_count++;
        CAN_CancelTransmit(CAN1, mailbox);
    }
}

int main(void)
{
    uint16_t current_centi_ma[ADC420_CHANNEL_COUNT];
    uint8_t frame[8];
    uint8_t frame_index;
    uint8_t i;
    CanTxMsg tx;

    SystemInit();
    SystemCoreClockUpdate();

    ADC420_Init();
    if (CAN1_Init_500K() == 0U) {
        while (1) { }
    }

    while (1) {
        ADC420_ScanAll(current_centi_ma);

        /* Three standard-ID frames; each current value is in 0.01mA units. */
        for (frame_index = 0U; frame_index < 3U; ++frame_index) {
            uint8_t count = (frame_index == 2U) ? 2U : 4U;

            for (i = 0U; i < 8U; ++i) {
                frame[i] = 0xFFU;
            }

            for (i = 0U; i < count; ++i) {
                uint16_t value = current_centi_ma[(uint8_t)(frame_index * 4U + i)];
                frame[2U * i] = (uint8_t)(value & 0xFFU);
                frame[2U * i + 1U] = (uint8_t)(value >> 8);
            }

            tx.StdId = (uint32_t)(0x301U + frame_index);
            tx.ExtId = 0U;
            tx.IDE = CAN_Id_Standard;
            tx.RTR = CAN_RTR_Data;
            tx.DLC = 8U;
            for (i = 0U; i < 8U; ++i) {
                tx.Data[i] = frame[i];
            }

            CAN1_SendFrame(&tx);
        }

        DelayMs(100U);
    }
}
