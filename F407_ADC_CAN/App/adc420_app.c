#include "adc420_app.h"
#include "adc420_spi.h"
#include "adc420_can.h"
#include <stdint.h>

/* Mapping follows "使用说明" section 2 and the board schematic.
   The board routes N_O1/N_O2/N_O3 to different active levels. */
const ADC420_Mapping adc420_map[ADC420_COUNT] = {
    { 2U, ADC420_CHIP_O2, 2U }, /* P10, ADC_2, N_O2 AIN2, PB1 HIGH */
    { 3U, ADC420_CHIP_O2, 3U }, /* P11, ADC_3, N_O2 AIN3, PB1 HIGH */
    { 4U, ADC420_CHIP_O1, 0U }, /* P1,  ADC_4, N_O1 AIN0, PB0 LOW */
    { 5U, ADC420_CHIP_O1, 1U }, /* P2,  ADC_5, N_O1 AIN1, PB0 LOW */
    { 6U, ADC420_CHIP_O1, 3U }, /* P4,  ADC_6, N_O1 AIN3, PB0 LOW */
    { 7U, ADC420_CHIP_O1, 2U }, /* P5,  ADC_7, N_O1 AIN2, PB0 LOW */
    { 8U, ADC420_CHIP_O3, 2U }, /* P9,  ADC_8, N_O3 AIN2, PB10 HIGH */
    { 9U, ADC420_CHIP_O3, 1U }, /* P7,  ADC_9, N_O3 AIN1, PB10 HIGH */
    {10U, ADC420_CHIP_O3, 3U }, /* P8,  ADC_10,N_O3 AIN3, PB10 HIGH */
    {11U, ADC420_CHIP_O3, 0U }  /* P6,  ADC_11,N_O3 AIN0, PB10 HIGH */
};

volatile int16_t adc420_raw[ADC420_COUNT];
volatile uint16_t adc420_current_ua[ADC420_COUNT];
volatile uint16_t adc420_valid_mask = 0U;
volatile uint16_t adc420_fault_mask = 0U;
volatile uint8_t adc420_sequence = 0U;
volatile uint32_t adc420_scan_count = 0UL;
volatile uint8_t adc420_last_can_failures = 0U;

/* ADS1118 +/-2.048 V: 1 count = 62.5 uV.
   With 100 ohms, 1 count = 0.625 uA. */
uint16_t ADC420_Convert_uA(int16_t raw)
{
    if (raw <= 0) return 0U;
    return (uint16_t)((((uint32_t)(uint16_t)raw * 625UL) + 500UL) / 1000UL);
}

static void adc420_pack_le16(uint8_t *dest, uint8_t offset, uint16_t value)
{
    dest[offset] = (uint8_t)value;
    dest[(uint8_t)(offset + 1U)] = (uint8_t)(value >> 8);
}

void ADC420_App_ScanAndSend(void)
{
    uint8_t i, group, n, data[8], errors = 0U;
    uint16_t valid = 0U, fault = 0U;
    int16_t sample;
    uint8_t good;

    for (i = 0U; i < ADC420_COUNT; i++) {
        good = ADC420_SPI_Read(adc420_map[i].chip, adc420_map[i].ain, &sample);
        if (good && sample >= 0) {
            adc420_raw[i] = sample;
            adc420_current_ua[i] = ADC420_Convert_uA(sample);
            valid |= (uint16_t)(1U << i);
            if (adc420_current_ua[i] < 3600U ||
                adc420_current_ua[i] > 21000U) {
                fault |= (uint16_t)(1U << i);
            }
        } else {
            adc420_raw[i] = (int16_t)-32768;
            adc420_current_ua[i] = 0xFFFFU;
            fault |= (uint16_t)(1U << i);
        }
    }

    adc420_valid_mask = valid;
    adc420_fault_mask = fault;
    adc420_sequence++;
    adc420_scan_count++;

    /* CAN 0x310..0x312: 10 x uint16 microamps, little endian. */
    for (group = 0U; group < 3U; group++) {
        n = (group == 2U) ? 2U : 4U;
        for (i = 0U; i < 8U; i++) data[i] = 0U;
        for (i = 0U; i < n; i++) {
            adc420_pack_le16(data, (uint8_t)(i * 2U),
                             adc420_current_ua[(uint8_t)(group * 4U + i)]);
        }
        if (!ADC420_CAN_Send((uint16_t)(ADC420_CAN_ID_0 + group),
                             data, (uint8_t)(n * 2U))) {
            errors++;
        }
    }

    adc420_last_can_failures = errors;
    adc420_pack_le16(data, 0U, valid);
    adc420_pack_le16(data, 2U, fault);
    data[4] = adc420_sequence;
    data[5] = errors;
    data[6] = 1U; /* protocol version */
    data[7] = 0U;
    (void)ADC420_CAN_Send(ADC420_CAN_ID_STATUS, data, 8U);
}
