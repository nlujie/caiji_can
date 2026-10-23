#include "stm32f10x.h"
#include "../Hardware/adc420_spi.h"
#include "../Hardware/adc420_can.h"
#include "../Hardware/adc420_app.h"

/* Only one SysTick owner; do NOT run legacy func.c/timer code. */
extern volatile uint32_t adc420_ms;
volatile uint8_t adc420_boot_error; /* 1 => SysTick_Config failed */

int main(void)
{
    uint32_t last_tick;
    /* startup_stm32f10x_md.s already calls SystemInit(). */
    SystemCoreClockUpdate();
    if (SysTick_Config(SystemCoreClock / 1000UL) != 0U) {
        adc420_boot_error = 1U;
        while (1) { }
    }
    ADC420_SPI_Init();
    ADC420_CAN_Init();
    /* First scan immediately; following scans have 100ms starting period. */
    last_tick = adc420_ms - ADC420_PERIOD_MS;
    for (;;) {
        if ((uint32_t)(adc420_ms-last_tick) >= ADC420_PERIOD_MS) {
            last_tick = adc420_ms;
            ADC420_App_ScanAndSend();
        }
    }
}
