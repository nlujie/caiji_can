#include "adc420_app.h"
#include "adc420_ads1118.h"
#include "adc420_can.h"
#include <stdint.h>

extern uint32_t ADC420_Millis(void);

/* Source: "采集板使用说明", section 2; original board schematic.
 * Order in the table = order on CAN bus:
 * ADC_2/P10 = N_O2/AIN2, ADC_3/P11 = N_O2/AIN3
 * ADC_4/P1 = N_O1/AIN0, ADC_5/P2 = N_O1/AIN1
 * ADC_6/P4 = N_O1/AIN3, ADC_7/P5 = N_O1/AIN2
 * ADC_8/P9 = N_O3/AIN2, ADC_9/P7 = N_O3/AIN1
 * ADC_10/P8 = N_O3/AIN3, ADC_11/P6 = N_O3/AIN0
 */
const ADC420_ChannelMap adc420_channel_map[ADC420_SENSOR_COUNT] = {
    {2U, ADC420_CHIP_O2, 2U}, {3U, ADC420_CHIP_O2, 3U},
    {4U, ADC420_CHIP_O1, 0U}, {5U, ADC420_CHIP_O1, 1U},
    {6U, ADC420_CHIP_O1, 3U}, {7U, ADC420_CHIP_O1, 2U},
    {8U, ADC420_CHIP_O3, 2U}, {9U, ADC420_CHIP_O3, 1U},
    {10U, ADC420_CHIP_O3, 3U}, {11U, ADC420_CHIP_O3, 0U}
};

volatile int16_t adc420_raw[ADC420_SENSOR_COUNT];
volatile uint16_t adc420_current_ua[ADC420_SENSOR_COUNT];
volatile uint16_t adc420_valid_mask;
volatile uint16_t adc420_fault_mask;
volatile uint8_t adc420_sequence;
volatile uint8_t adc420_can_error_count;

/* ADS1118 +/-2.048 V => 62.5 uV/LSB.
 * 100 ohm shunt => 0.625 uA/LSB; round to nearest 1 uA.
 * 4 mA = raw 6400, 12 mA = 19200, 20 mA = 32000.
 * Only nonnegative raw codes are accepted on unipolar shunt inputs. */
uint16_t ADC420_RawToMicroamps(int16_t raw)
{
    if (raw <= 0) return 0U;
    return (uint16_t)(((uint32_t)(uint16_t)raw * 625U + 500U) / 1000U);
}

static void adc420_pack_word(uint8_t *data, uint8_t offset, uint16_t v)
{
    data[offset] = (uint8_t)v;                 /* little-endian */
    data[(uint8_t)(offset + 1U)] = (uint8_t)(v >> 8);
}

void ADC420_App_RunCycle(uint8_t can_ready)
{
    uint8_t i, group, payload[8], frame_failures = 0U;
    uint16_t valid_mask = 0U, fault_mask = 0U;
    uint32_t started = ADC420_Millis();
    uint32_t duration;
    int16_t raw;
    uint16_t ua;

    for (i = 0U; i < ADC420_SENSOR_COUNT; ++i) {
        uint8_t ok = ADC420_ADS1118_Read(adc420_channel_map[i].ads_chip,
                                           adc420_channel_map[i].ads_ain, &raw);
        if (ok && raw >= 0) {
            ua = ADC420_RawToMicroamps(raw);
            adc420_raw[i] = raw;
            adc420_current_ua[i] = ua;
            valid_mask |= (uint16_t)(1U << i);
            if (ua < 3600U || ua > 21000U) fault_mask |= (uint16_t)(1U << i);
        } else {
            adc420_raw[i] = 0;
            adc420_current_ua[i] = 0xFFFFU; /* invalid sentinel */
            fault_mask |= (uint16_t)(1U << i);
        }
    }

    adc420_valid_mask = valid_mask;
    adc420_fault_mask = fault_mask;
    adc420_sequence++;

    /* Three sample frames: 4 + 4 + 2 channels, respectively.
     * NOTE: invalid values are 0xFFFF, not zero current. */
    for (group = 0U; group < 3U; ++group) {
        uint8_t n = (group == 2U) ? 2U : 4U;
        for (i = 0U; i < n; ++i) {
            adc420_pack_word(payload, (uint8_t)(i * 2U),
                             adc420_current_ua[(uint8_t)(group * 4U + i)]);
        }
        if (can_ready) {
            if (!ADC420_CAN_Send((uint16_t)(ADC420_CAN_FRAME_0 + group),
                                 payload, (uint8_t)(n * 2U))) frame_failures++;
        }
    }

    duration = ADC420_Millis() - started;
    payload[0] = (uint8_t)valid_mask;
    payload[1] = (uint8_t)(valid_mask >> 8);
    payload[2] = (uint8_t)fault_mask;
    payload[3] = (uint8_t)(fault_mask >> 8);
    payload[4] = adc420_sequence;
    payload[5] = frame_failures; /* failed data frames this cycle */
    payload[6] = duration > 255U ? 255U : (uint8_t)duration;
    payload[7] = 1U; /* protocol version */
    if (can_ready) {
        if (!ADC420_CAN_Send(ADC420_CAN_STATUS, payload, 8U)) frame_failures++;
    }
    adc420_can_error_count = frame_failures;
}
