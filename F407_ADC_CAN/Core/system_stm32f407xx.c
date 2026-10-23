#include "stm32f407_min.h"

uint32_t SystemCoreClock = 168000000UL;

/*
 * Board schematic: 8 MHz HSE crystal.
 * Use PLL M=8, N=336, P=2 for 168 MHz SYSCLK.
 * APB1=/4 (42 MHz, CAN1) and APB2=/2 (84 MHz, SPI1).
 * If HSE/PLL does not start, fall back to 16 MHz HSI.
 */
void SystemInit(void)
{
    uint32_t timeout;

    RCC_CR |= (1UL << 0); /* HSI on */
    for (timeout = 0UL; timeout < 1000000UL; ++timeout) {
        if ((RCC_CR & (1UL << 1)) != 0UL) break;
    }

    RCC_APB1ENR |= (1UL << 28); /* PWR clock */
    PWR_CR = (PWR_CR & ~(3UL << 14)) | (3UL << 14); /* voltage scale 1 */
    FLASH_ACR = 5UL | (1UL << 8) | (1UL << 9) | (1UL << 10);

    RCC_CR |= (1UL << 16); /* HSE on */
    for (timeout = 0UL; timeout < 1000000UL; ++timeout) {
        if ((RCC_CR & (1UL << 17)) != 0UL) break;
    }

    if ((RCC_CR & (1UL << 17)) != 0UL) {
        RCC_PLLCFGR = 8UL | (336UL << 6) | (1UL << 22) | (7UL << 24);
        RCC_CR |= (1UL << 24); /* PLL on */
        for (timeout = 0UL; timeout < 1000000UL; ++timeout) {
            if ((RCC_CR & (1UL << 25)) != 0UL) break;
        }

        if ((RCC_CR & (1UL << 25)) != 0UL) {
            RCC_CFGR = (RCC_CFGR & ~((7UL << 10) | (7UL << 13) | 3UL))
                     | (5UL << 10) | (4UL << 13); /* APB1=/4, APB2=/2 */
            RCC_CFGR = (RCC_CFGR & ~3UL) | 2UL;   /* SYSCLK=PLL */
            for (timeout = 0UL; timeout < 1000000UL; ++timeout) {
                if ((RCC_CFGR & (3UL << 2)) == (2UL << 2)) break;
            }
            if ((RCC_CFGR & (3UL << 2)) == (2UL << 2)) {
                SystemCoreClock = 168000000UL;
                return;
            }
        }
    }

    /* Safe fallback: HSI, AHB/APB prescalers = 1. */
    RCC_CR &= ~(1UL << 24);
    RCC_CFGR &= ~((7UL << 10) | (7UL << 13) | 3UL);
    RCC_CR |= (1UL << 0);
    for (timeout = 0UL; timeout < 1000000UL; ++timeout) {
        if ((RCC_CR & (1UL << 1)) != 0UL) break;
    }
    SystemCoreClock = 16000000UL;
}

void SystemCoreClockUpdate(void)
{
    uint32_t sws = RCC_CFGR & (3UL << 2);
    if (sws == (2UL << 2)) {
        SystemCoreClock = 168000000UL;
    } else if (sws == (1UL << 2)) {
        SystemCoreClock = 8000000UL;
    } else {
        SystemCoreClock = 16000000UL;
    }
}
