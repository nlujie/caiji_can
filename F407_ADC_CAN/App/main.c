#include "stm32f407_min.h"
#include "timebase.h"
#include "adc420_spi.h"
#include "adc420_can.h"
#include "adc420_app.h"

int main(void)
{
    uint32_t scan_start;

    SystemCoreClockUpdate();
    Timebase_Init();
    ADC420_SPI_Init();
    (void)ADC420_CAN_Init();

    for (;;) {
        scan_start = Timebase_Millis();
        ADC420_App_ScanAndSend();

        while ((uint32_t)(Timebase_Millis() - scan_start) < ADC420_PERIOD_MS) {
            /* fixed 100 ms scan cadence */
        }
    }
}
