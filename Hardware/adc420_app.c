#include "adc420_app.h"
#include "adc420_spi.h"
#include "adc420_can.h"
#include <stdint.h>

/* 10 inputs, from user's original PCB and section 2 of board instructions.
   P6 note: manual's "11/12" pin labels appear copied from P9.
   Its actual connector is P6, two pins. */
const ADC420_Mapping adc420_map[ADC420_COUNT] = {
    { 2U, ADC420_CHIP_O2, 2U }, /* P10, N_O2 AIN2, PB1 active HIGH */
    { 3U, ADC420_CHIP_O2, 3U }, /* P11, N_O2 AIN3 */
    { 4U, ADC420_CHIP_O1, 0U }, /* P1, N_O1 AIN0, PB0 active LOW */
    { 5U, ADC420_CHIP_O1, 1U }, /* P2, N_O1 AIN1 */
    { 6U, ADC420_CHIP_O1, 3U }, /* P4, N_O1 AIN3 */
    { 7U, ADC420_CHIP_O1, 2U }, /* P5, N_O1 AIN2 */
    { 8U, ADC420_CHIP_O3, 2U }, /* P9, N_O3 AIN2, PB10 active HIGH */
    { 9U, ADC420_CHIP_O3, 1U }, /* P7, N_O3 AIN1 */
    {10U, ADC420_CHIP_O3, 3U }, /* P8, N_O3 AIN3 */
    {11U, ADC420_CHIP_O3, 0U }  /* P6, N_O3 AIN0 */
};
volatile int16_t adc420_raw[ADC420_COUNT];
volatile uint16_t adc420_current_ua[ADC420_COUNT];
volatile uint16_t adc420_valid_mask;
volatile uint16_t adc420_fault_mask;
volatile uint8_t adc420_sequence;
volatile uint32_t adc420_scan_count;
volatile uint8_t adc420_last_can_failures;

/* ADS1118 range +/-2.048 V; each ADC count is 62.5 uV.
   Rshunt=100 ohms; current = 0.625 uA/count.
   raw=6400 => 4000uA, 19200 => 12000uA, 32000 => 20000uA. */
uint16_t ADC420_Convert_uA(int16_t raw)
{
    if (raw <= 0) return 0U;
    return (uint16_t)((((uint32_t)(uint16_t)raw * 625UL) + 500UL) / 1000UL);
}
static void adc420_le16(uint8_t *dest, uint8_t offset, uint16_t x)
{
    dest[offset] = (uint8_t)x;
    dest[(uint8_t)(offset+1U)] = (uint8_t)(x >> 8);
}
void ADC420_App_ScanAndSend(void)
{
    uint8_t i, group, n, data[8], errors=0U;
    uint16_t mask=0U, fault=0U;
    uint16_t ua;
    int16_t sample;
    uint8_t good;
    for (i=0U; i<ADC420_COUNT; i++) {
        good = ADC420_SPI_Read(adc420_map[i].chip, adc420_map[i].ain, &sample);
        if (good && sample >= 0) {
            adc420_raw[i] = sample;
            ua = ADC420_Convert_uA(sample);
            adc420_current_ua[i] = ua;
            mask |= (uint16_t)(1U << i);
            if (ua < 3600U || ua > 21000U) fault |= (uint16_t)(1U << i);
        } else {
            adc420_raw[i] = (int16_t)-32768;
            adc420_current_ua[i] = 0xFFFFU;
            fault |= (uint16_t)(1U << i);
        }
    }
    adc420_valid_mask = mask; /* SPI success != analog 4-20mA in range */
    adc420_fault_mask = fault;
    adc420_sequence++;
    adc420_scan_count++;

    /* 10 channels * 2 bytes (uA), little endian, 3 CAN data frames. */
    for (group=0U; group<3U; group++) {
        n = (group==2U) ? 2U : 4U;
        for (i=0U; i<n; i++) {
            adc420_le16(data, (uint8_t)(i*2U),
                        adc420_current_ua[(uint8_t)(group*4U+i)]);
        }
        if (!ADC420_CAN_Send((uint16_t)(ADC420_CAN_ID_0+group), data,
                             (uint8_t)(n*2U))) errors++;
    }
    adc420_last_can_failures = errors;
    adc420_le16(data, 0U, mask);
    adc420_le16(data, 2U, fault);
    data[4] = adc420_sequence;
    data[5] = errors;
    data[6] = 1U; /* protocol version */
    data[7] = 0U; /* reserved */
    (void)ADC420_CAN_Send(ADC420_CAN_ID_STATUS, data, 8U);
}
