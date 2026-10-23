#include "stm32f10x.h"
#include "../Hardware/adc420_ads1118.h"
#include "../Hardware/adc420_can.h"
#include "../Hardware/adc420_app.h"

/* Keil ARMCC5-compatible minimal 4-20mA acquisition firmware.
 * 10 channels, external ADS1118 SPI ADC, CAN1 at 500kbps.
 * No temperature/PT100/PWM/Modbus acquisition is performed here. */
extern volatile uint32_t adc420_tick_ms;
uint32_t ADC420_Millis(void)
{
    return adc420_tick_ms;
}

int main(void)
{
    uint32_t last_cycle;
    uint8_t can_ready;

    SystemInit();
    if (SysTick_Config(SystemCoreClock / 1000U) != 0U) {
        while (1) { }
    }

    ADC420_ADS1118_Init();
    /* Verify SPI chip links using internal sensors before reading external inputs. */
    (void)ADC420_ADS1118_SelfTest();
    can_ready = ADC420_CAN_Init();
    last_cycle = ADC420_Millis() - ADC420_CAN_PERIOD_MS;
    while (1) {
        uint32_t now = ADC420_Millis();
        if ((uint32_t)(now - last_cycle) >= ADC420_CAN_PERIOD_MS) {
            last_cycle = now;
            ADC420_App_RunCycle(can_ready);
        }
    }
}
