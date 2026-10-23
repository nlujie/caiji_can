#include "adc420_can.h"
#include "stm32f407_min.h"
#include "timebase.h"

volatile uint8_t adc420_can_initialized = 0U;
volatile uint32_t adc420_can_tx_ok = 0UL;
volatile uint32_t adc420_can_tx_failed = 0UL;

uint8_t ADC420_CAN_Init(void)
{
    uint32_t timeout;
    uint32_t bs1, bs2, brp;
    uint32_t pinmask;

    RCC_AHB1ENR |= (1UL << 0);  /* GPIOA */
    RCC_APB1ENR |= (1UL << 25); /* CAN1 */
    (void)RCC_APB1ENR;

    /* STM32F407 CAN1: PA11=RX, PA12=TX, AF9. */
    pinmask = (3UL << 22) | (3UL << 24);
    GPIOA_MODER = (GPIOA_MODER & ~pinmask) | (2UL << 22) | (2UL << 24);
    GPIOA_OTYPER &= ~((1UL << 11) | (1UL << 12));
    GPIOA_OSPEEDR = (GPIOA_OSPEEDR & ~pinmask) |
                    (3UL << 22) | (3UL << 24);
    GPIOA_PUPDR = (GPIOA_PUPDR & ~pinmask) | (1UL << 22); /* RX pull-up */
    GPIOA_AFRH = (GPIOA_AFRH & ~((15UL << 12) | (15UL << 16))) |
                 (9UL << 12) | (9UL << 16); /* AF9: CAN1 */

    CAN1_MCR = 1UL | (1UL << 5) | (1UL << 6); /* init, AWUM, bus-off recovery */
    for (timeout = 0UL; timeout < 1000000UL; ++timeout) {
        if ((CAN1_MSR & 1UL) != 0UL) break; /* INAK */
    }
    if ((CAN1_MSR & 1UL) == 0UL) {
        adc420_can_initialized = 0U;
        return 0U;
    }

    /* 500 kbit/s, sample point ~86-88%, normal mode.
       PLL clock: APB1=42 MHz, BRP=6, BS1=11 TQ, BS2=2 TQ.
       HSI fallback: APB1=16 MHz, BRP=2, BS1=13 TQ, BS2=2 TQ. */
    if (SystemCoreClock >= 100000000UL) {
        brp = 5UL;
        bs1 = 10UL;
        bs2 = 1UL;
    } else {
        brp = 1UL;
        bs1 = 12UL;
        bs2 = 1UL;
    }
    CAN1_BTR = (bs2 << 20) | (bs1 << 16) | brp;

    CAN1_MCR = (1UL << 5) | (1UL << 6); /* leave init mode */
    for (timeout = 0UL; timeout < 1000000UL; ++timeout) {
        if ((CAN1_MSR & 1UL) == 0UL) break;
    }
    adc420_can_initialized = ((CAN1_MSR & 1UL) == 0UL) ? 1U : 0U;
    return adc420_can_initialized;
}

uint8_t ADC420_CAN_Send(uint16_t id, const uint8_t *data, uint8_t len)
{
    uint8_t mailbox;
    uint8_t i;
    uint32_t tsr, start, rqcp, txok, abrq, timeout;
    volatile uint32_t *mbx;

    if (!adc420_can_initialized || data == 0 || len > 8U || id > 0x7FFU) {
        adc420_can_tx_failed++;
        return 0U;
    }

    tsr = CAN1_TSR;
    if ((tsr & (1UL << 26)) != 0UL) mailbox = 0U;
    else if ((tsr & (1UL << 27)) != 0UL) mailbox = 1U;
    else if ((tsr & (1UL << 28)) != 0UL) mailbox = 2U;
    else {
        adc420_can_tx_failed++;
        return 0U;
    }

    mbx = CAN1_TX_MAILBOX(mailbox);
    mbx[0] = ((uint32_t)id << 21); /* standard data frame */
    mbx[1] = (uint32_t)(len & 0x0FU);
    mbx[2] = (uint32_t)data[0]
           | ((uint32_t)((len > 1U) ? data[1] : 0U) << 8)
           | ((uint32_t)((len > 2U) ? data[2] : 0U) << 16)
           | ((uint32_t)((len > 3U) ? data[3] : 0U) << 24);
    mbx[3] = (uint32_t)((len > 4U) ? data[4] : 0U)
           | ((uint32_t)((len > 5U) ? data[5] : 0U) << 8)
           | ((uint32_t)((len > 6U) ? data[6] : 0U) << 16)
           | ((uint32_t)((len > 7U) ? data[7] : 0U) << 24);
    mbx[0] = ((uint32_t)id << 21) | 1UL; /* TXRQ */

    rqcp = 1UL << ((uint32_t)mailbox * 8UL);
    txok = 1UL << (((uint32_t)mailbox * 8UL) + 1UL);
    abrq = 1UL << (((uint32_t)mailbox * 8UL) + 7UL);
    start = Timebase_Millis();
    while ((uint32_t)(Timebase_Millis() - start) < 8UL) {
        tsr = CAN1_TSR;
        if ((tsr & rqcp) != 0UL) {
            uint8_t success = ((tsr & txok) != 0UL) ? 1U : 0U;
            CAN1_TSR = rqcp; /* clear RQCP and associated status bits */
            if (success) adc420_can_tx_ok++;
            else adc420_can_tx_failed++;
            return success;
        }
    }

    /* Avoid occupying all mailboxes if the bus has no active ACKing node. */
    CAN1_TSR = abrq;
    for (timeout = 0UL; timeout < 200000UL; ++timeout) {
        if ((CAN1_TSR & rqcp) != 0UL) break;
    }
    if ((CAN1_TSR & rqcp) != 0UL) CAN1_TSR = rqcp;
    adc420_can_tx_failed++;
    return 0U;
}
