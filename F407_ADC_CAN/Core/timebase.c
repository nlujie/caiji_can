#include "stm32f407_min.h"
#include "timebase.h"

volatile uint32_t adc420_ms = 0UL;

void Timebase_Init(void)
{
    SystemCoreClockUpdate();
    SYST_CSR = 0UL;
    SYST_RVR = (SystemCoreClock / 1000UL) - 1UL;
    SYST_CVR = 0UL;
    SYST_CSR = 7UL; /* processor clock, interrupt enabled, counter enabled */
}

void SysTick_Handler(void)
{
    adc420_ms++;
}

uint32_t Timebase_Millis(void)
{
    return adc420_ms;
}

void Delay_ms(uint32_t ms)
{
    uint32_t start = Timebase_Millis();
    while ((uint32_t)(Timebase_Millis() - start) < ms) {
        /* wait for SysTick */
    }
}
