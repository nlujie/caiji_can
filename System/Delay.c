#include "Delay.h"
#include "stm32f10x.h"

/* Cortex-M3 DWT cycle counter: delays must not overwrite SysTick->CTRL/LOAD. */
#define DELAY_DWT_CTRL   (*((volatile uint32_t *)0xE0001000UL))
#define DELAY_DWT_COUNT  (*((volatile uint32_t *)0xE0001004UL))
static uint8_t counter_ready = 0U;
static uint8_t counter_checked = 0U;

static void Delay_InitCounter(void)
{
    uint32_t before;
    volatile uint32_t k;
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DELAY_DWT_COUNT = 0U;
    DELAY_DWT_CTRL |= 1U;
    before = DELAY_DWT_COUNT;
    for(k = 0U; k < 100U; k++) { __NOP(); }
    counter_ready = (DELAY_DWT_COUNT != before) ? 1U : 0U;
    counter_checked = 1U;
}

void Delay_us(uint32_t us)
{
    uint32_t clock_per_us, chunk, start;
    volatile uint32_t wait_count;
    if(us == 0U) return;
    if(!counter_checked) Delay_InitCounter();

    clock_per_us = SystemCoreClock / 1000000U;
    if(clock_per_us == 0U) clock_per_us = 1U;
    if(!counter_ready)
    {
        while(us-- != 0U)
        {
            for(wait_count = 0U; wait_count < clock_per_us / 4U + 1U; wait_count++)
                __NOP();
        }
        return;
    }
    while(us != 0U)
    {
        chunk = (us > 250000UL) ? 250000UL : us;
        start = DELAY_DWT_COUNT;
        while((uint32_t)(DELAY_DWT_COUNT - start) < chunk * clock_per_us) { }
        us -= chunk;
    }
}

void Delay_ms(uint32_t ms)
{
    while(ms-- != 0U) Delay_us(1000U);
}

void Delay_s(uint32_t seconds)
{
    while(seconds-- != 0U) Delay_ms(1000U);
}
