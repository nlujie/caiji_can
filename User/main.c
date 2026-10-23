#include "stm32f10x.h"
#include "../Hardware/adc420_spi.h"
#include "../Hardware/adc420_can.h"
#include "../Hardware/adc420_app.h"

extern volatile uint32_t adc420_ms;

/* Keil Watch diagnostics: startup and main-loop state. */
volatile uint8_t adc420_boot_error = 0U;
volatile uint8_t adc420_debug_init_stage = 0U;
volatile uint32_t adc420_debug_system_clock_hz = 0UL;
volatile uint8_t adc420_debug_spi_init_ok = 0U;
volatile uint8_t adc420_debug_can_init_ok = 0U;
volatile uint32_t adc420_debug_main_loop_count = 0UL;

int main(void)
{
    uint32_t last_tick;

    /* STM32F103 startup file calls SystemInit() before main(). */
    SystemCoreClockUpdate();
    adc420_debug_system_clock_hz = SystemCoreClock;
    adc420_debug_init_stage = 1U;

    if (SysTick_Config(SystemCoreClock / 1000UL) != 0U) {
        adc420_boot_error = 1U;
        adc420_debug_init_stage = 0xE1U;
        while (1) { }
    }
    adc420_debug_init_stage = 2U;

    ADC420_SPI_Init();
    adc420_debug_spi_init_ok = adc420_debug_spi_init_done;
    adc420_debug_init_stage = 3U;

    adc420_debug_can_init_ok = ADC420_CAN_Init();
    adc420_debug_init_stage = 4U;

    /* Trigger the first scan immediately; then scan every 100 ms. */
    last_tick = adc420_ms - ADC420_PERIOD_MS;
    for (;;) {
        if ((uint32_t)(adc420_ms - last_tick) >= ADC420_PERIOD_MS) {
            last_tick = adc420_ms;
            adc420_debug_init_stage = 5U;
            ADC420_App_ScanAndSend();
            adc420_debug_main_loop_count++;
        }
    }
}
