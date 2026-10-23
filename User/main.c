#include "stm32f10x.h"
#include "system_stm32f10x.h"
#include "Delay.h"
#include "ads1118_spi.h"
#include "can.h"
#include "adc420_app.h"

/*
 * Application scope:
 *   - STM32F103C8T6
 *   - Three ADS1118 devices on one shared SPI1 bus
 *   - Ten 4-20 mA channels (100 ohm shunt -> 0.4-2.0 V)
 *   - Five standard CAN frames per complete scan
 *
 * SystemInit() is also called by the startup file; calling it here keeps
 * the intended 72 MHz system clock explicit for this standalone project.
 */
int main(void)
{
    uint8_t can_ok;

    SystemInit();
    SystemCoreClockUpdate();

    ADS1118_Init();
    ADC420_Init();
    can_ok = CAN_Module_Init();

    while (1)
    {
        /*
         * A complete scan takes approximately 20-25 ms because each ADS1118
         * conversion is allowed 2 ms to finish. With the delay below, the
         * full scan-and-send cycle is approximately 100 ms.
         */
        if (ADC420_ScanAll() != 0U)
        {
            if (can_ok != 0U)
            {
                (void)ADC420_SendCanFrames();
            }
        }

        Delay_ms(80U);
    }
}
