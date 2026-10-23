#include "stm32f407_min.h"
#include "timebase.h"
#include "adc420_spi.h"
#include "adc420_can.h"
#include "adc420_app.h"

/* Global startup diagnostics, visible in Keil Watch. */
volatile uint8_t adc420_debug_init_stage = 0U;
volatile uint32_t adc420_debug_system_clock_hz = 0UL;
volatile uint8_t adc420_debug_spi_init_ok = 0U;
volatile uint8_t adc420_debug_can_init_ok = 0U;
volatile uint32_t adc420_debug_main_loop_count = 0UL;

int main(void)
{
    uint32_t scan_start;

    SystemCoreClockUpdate();
    adc420_debug_system_clock_hz = SystemCoreClock;
    adc420_debug_init_stage = 1U; /* clock configured */

    Timebase_Init();
    adc420_debug_init_stage = 2U; /* 1 ms timebase running */

    ADC420_SPI_Init();
    adc420_debug_spi_init_ok = 1U;
    adc420_debug_init_stage = 3U; /* SPI1 GPIO/peripheral configured */

    adc420_debug_can_init_ok = ADC420_CAN_Init();
    adc420_debug_init_stage = 4U; /* CAN init attempt complete */

    for (;;) {
        adc420_debug_init_stage = 5U; /* main loop running */
        adc420_debug_main_loop_count++;
        scan_start = Timebase_Millis();
        ADC420_App_ScanAndSend();

        while ((uint32_t)(Timebase_Millis() - scan_start) < ADC420_PERIOD_MS) {
            /* fixed 100 ms scan cadence */
        }
    }
}
