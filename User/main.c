#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_spi.h"
#include "stm32f10x_can.h"
#include "adc420_app.h"

static void ClockDelay(volatile uint32_t n)
{
    while (n--) { __NOP(); }
}

static void DelayMs(uint32_t ms)
{
    while (ms--) ClockDelay(SystemCoreClock / 8000U);
}

static void CAN1_Init_500K(void)
{
    GPIO_InitTypeDef gpio;
    CAN_InitTypeDef can;
    CAN_FilterInitTypeDef filter;
    NVIC_InitTypeDef nvic;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);

    /* Board connector U25: CANH/CANL; MCU CAN1 uses PA12(TX)/PA11(RX). */
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
    can.CAN_NART = DISABLE;
    can.CAN_RFLM = DISABLE;
    can.CAN_TXFP = ENABLE;
    can.CAN_Mode = CAN_Mode_Normal;
    can.CAN_SJW = CAN_SJW_1tq;
    can.CAN_BS1 = CAN_BS1_9tq;
    can.CAN_BS2 = CAN_BS2_8tq;
    can.CAN_Prescaler = 4; /* PCLK1=36MHz; 36MHz/(4*18TQ)=500kbps */
    if (CAN_Init(CAN1, &can) == CANINITFAILED) {
        while (1) { }
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

    nvic.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 2;
    nvic.NVIC_IRQChannelSubPriority = 0;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);
}

int main(void)
{
    uint16_t current_centi_ma[ADC420_CHANNEL_COUNT];
    uint8_t frame[8];
    uint8_t frame_index, i;
    CanTxMsg tx;

    SystemInit();
    SystemCoreClockUpdate();
    ADC420_Init();
    CAN1_Init_500K();

    while (1) {
        ADC420_ScanAll(current_centi_ma);

        /* Three standard-ID frames: four channels per frame, units=0.01mA.
           ADC2..ADC11 map in ascending logical order; final frame carries 2 channels. */
        for (frame_index = 0; frame_index < 3; ++frame_index) {
            uint8_t count = (frame_index == 2U) ? 2U : 4U;
            for (i = 0; i < 8U; ++i) frame[i] = 0xFFU;
            for (i = 0; i < count; ++i) {
                uint16_t value = current_centi_ma[frame_index * 4U + i];
                frame[2U * i] = (uint8_t)(value & 0xFFU);
                frame[2U * i + 1U] = (uint8_t)(value >> 8);
            }
            tx.StdId = (uint32_t)(0x301U + frame_index);
            tx.ExtId = 0;
            tx.IDE = CAN_Id_Standard;
            tx.RTR = CAN_RTR_Data;
            tx.DLC = 8;
            for (i = 0; i < 8U; ++i) tx.Data[i] = frame[i];

            /* Bounded wait: don't freeze acquisition forever if bus is offline. */
            {
                uint32_t timeout = 100000U;
                uint8_t mailbox = CAN_Transmit(CAN1, &tx);
                if (mailbox != CAN_TxStatus_NoMailBox) {
                    while ((CAN_TransmitStatus(CAN1, mailbox) != CAN_TxStatus_Ok) &&
                           (CAN_TransmitStatus(CAN1, mailbox) != CAN_TxStatus_Failed) &&
                           timeout--) { }
                }
            }
        }
        DelayMs(100);
    }
}
