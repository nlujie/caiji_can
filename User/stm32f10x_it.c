#include "stm32f10x_it.h"
#include "stm32f10x.h"

/* Sole 1ms time base, used by ADS1118 and CAN bounded timeouts. */
volatile uint32_t adc420_tick_ms = 0U;

void NMI_Handler(void) {}
void HardFault_Handler(void) { while (1) { } }
void MemManage_Handler(void) { while (1) { } }
void BusFault_Handler(void) { while (1) { } }
void UsageFault_Handler(void) { while (1) { } }
void SVC_Handler(void) {}
void DebugMon_Handler(void) {}
void PendSV_Handler(void) {}
void SysTick_Handler(void) { ++adc420_tick_ms; }
