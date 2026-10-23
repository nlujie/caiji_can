#include "stm32f10x.h"
#include "system_stm32f10x.h"
#include "debug_uart.h"
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

    DebugUart_Init();
    DebugUart_Printf("\r\n[BOOT] STM32F103C8T6 ADC420/CAN debug started\r\n");
    DebugUart_Printf("[BOOT] USART1 TX=PA9, baud=115200, 8N1\r\n");

    ADS1118_Init();
    ADC420_Init();
    can_ok = CAN_Module_Init();

    DebugUart_Printf("[BOOT] ADS1118 SPI ready=%u, CAN init=%u\r\n",
                     (unsigned int)ads1118_spi_ready,
                     (unsigned int)can_ok);
    DebugUart_Printf("[BOOT] CAN baud=500 kbps, IDs=0x301..0x305\r\n");

    while (1)
    {
        (void)ADC420_ScanAll();
        ADC420_DebugPrintReadings();

        /*
         * ADC420_SendCanFrames prints an explicit SKIP message when the ADC
         * scan failed or CAN was not initialized. Successful/failed frame
         * attempts are printed with the exact eight-byte payload.
         */
        (void)ADC420_SendCanFrames();

        /* UART diagnostics add time; effective cycle rate is below 10 Hz. */
        Delay_ms(80U);
    }
}
