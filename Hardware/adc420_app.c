#include "adc420_app.h"
#include "ads1118_spi.h"
#include "can.h"
#include "debug_uart.h"

typedef struct
{
    uint8_t chip;
    uint8_t channel;
    uint8_t adc_id;
} ADC420_ChannelMap_t;

/*
 * Mapping transcribed from the supplied "采集板使用说明", section II:
 * ADC_2/3 = N_O2 AIN2/3 (PB1)
 * ADC_4/5/6/7 = N_O1 AIN0/1/3/2 (PB0)
 * ADC_8/9/10/11 = N_O3 AIN2/1/3/0 (PB10)
 */
static const ADC420_ChannelMap_t adc420_channel_map[ADC420_CHANNEL_COUNT] =
{
    { ADS1118_CHIP_N_O2, 2U,  2U },
    { ADS1118_CHIP_N_O2, 3U,  3U },
    { ADS1118_CHIP_N_O1, 0U,  4U },
    { ADS1118_CHIP_N_O1, 1U,  5U },
    { ADS1118_CHIP_N_O1, 3U,  6U },
    { ADS1118_CHIP_N_O1, 2U,  7U },
    { ADS1118_CHIP_N_O3, 2U,  8U },
    { ADS1118_CHIP_N_O3, 1U,  9U },
    { ADS1118_CHIP_N_O3, 3U, 10U },
    { ADS1118_CHIP_N_O3, 0U, 11U }
};

volatile int16_t adc420_raw[ADC420_CHANNEL_COUNT] = { 0 };
volatile float adc420_voltage_v[ADC420_CHANNEL_COUNT] = { 0.0f };
volatile float adc420_current_ma[ADC420_CHANNEL_COUNT] = { 0.0f };
volatile int16_t adc420_current_centi_ma[ADC420_CHANNEL_COUNT] = { 0 };
volatile uint8_t adc420_channel_valid[ADC420_CHANNEL_COUNT] = { 0U };

volatile uint8_t  adc420_scan_ok = 0U;
volatile uint32_t adc420_scan_count = 0U;
volatile uint32_t adc420_scan_fail_count = 0U;
volatile uint32_t adc420_can_frame_count = 0U;
volatile uint32_t adc420_can_error_count = 0U;
volatile uint16_t adc420_last_can_id = 0U;
volatile uint8_t  adc420_last_can_status = 0U;
volatile uint8_t  adc420_last_adc_id = 0U;
volatile uint8_t  adc420_last_chip = 0U;
volatile uint8_t  adc420_last_channel = 0U;

static void ADC420_PackInt16LE(uint8_t *dst, int16_t value)
{
    uint16_t bits;

    bits = (uint16_t)value;
    dst[0] = (uint8_t)(bits & 0x00FFU);
    dst[1] = (uint8_t)(bits >> 8);
}

void ADC420_Init(void)
{
    uint8_t i;

    for (i = 0U; i < ADC420_CHANNEL_COUNT; i++)
    {
        adc420_raw[i] = 0;
        adc420_voltage_v[i] = 0.0f;
        adc420_current_ma[i] = 0.0f;
        adc420_current_centi_ma[i] = 0;
        adc420_channel_valid[i] = 0U;
    }

    adc420_scan_ok = 0U;
    adc420_scan_count = 0U;
    adc420_scan_fail_count = 0U;
    adc420_can_frame_count = 0U;
    adc420_can_error_count = 0U;
    adc420_last_can_id = 0U;
    adc420_last_can_status = 0U;
    adc420_last_adc_id = 0U;
    adc420_last_chip = 0U;
    adc420_last_channel = 0U;
}

uint8_t ADC420_ScanAll(void)
{
    uint8_t i;
    uint8_t all_ok;
    int16_t raw;

    all_ok = 1U;

    for (i = 0U; i < ADC420_CHANNEL_COUNT; i++)
    {
        adc420_last_adc_id = adc420_channel_map[i].adc_id;
        adc420_last_chip = adc420_channel_map[i].chip;
        adc420_last_channel = adc420_channel_map[i].channel;

        raw = 0;
        if (ADS1118_ReadChannel(
                adc420_channel_map[i].chip,
                adc420_channel_map[i].channel,
                &raw) == 0U)
        {
            adc420_channel_valid[i] = 0U;
            adc420_raw[i] = 0;
            adc420_voltage_v[i] = 0.0f;
            adc420_current_ma[i] = 0.0f;
            adc420_current_centi_ma[i] = 0;
            adc420_scan_fail_count++;
            all_ok = 0U;
            continue;
        }

        adc420_raw[i] = raw;

        /*
         * PGA = +/-2.048 V: 1 LSB = 62.5 uV.
         * A 100 ohm shunt gives I(mA) = V(V) * 10.
         * 0.01 mA units equal raw / 16 for this resistor and PGA setting.
         * Values are not clamped to 4..20 mA so open/under-range faults remain visible.
         */
        adc420_voltage_v[i] = (float)raw * 0.0000625f;
        adc420_current_ma[i] = adc420_voltage_v[i] * 10.0f;
        adc420_current_centi_ma[i] = (int16_t)(raw / 16);
        adc420_channel_valid[i] = 1U;
    }

    adc420_scan_ok = all_ok;
    adc420_scan_count++;
    return all_ok;
}


/*
 * Print all ten readings in fixed-point form. Avoid %f so this works with the
 * standard Keil ARMCC library even when floating-point printf is disabled.
 */
void ADC420_DebugPrintReadings(void)
{
    uint8_t i;
    int32_t voltage_mv;
    int32_t current_centi_ma;
    uint32_t abs_voltage_mv;
    uint32_t abs_current_centi_ma;
    char voltage_sign;
    char current_sign;

    DebugUart_Printf("\r\n[ADC] scan=%lu status=%s\r\n",
                     (unsigned long)adc420_scan_count,
                     (adc420_scan_ok != 0U) ? "OK" : "FAIL");

    for (i = 0U; i < ADC420_CHANNEL_COUNT; i++)
    {
        voltage_mv = ((int32_t)adc420_raw[i] * 625L);
        if (voltage_mv >= 0)
        {
            voltage_mv = (voltage_mv + 5000L) / 10000L;
            voltage_sign = '+';
        }
        else
        {
            voltage_mv = (voltage_mv - 5000L) / 10000L;
            voltage_sign = '-';
        }

        current_centi_ma = (int32_t)adc420_current_centi_ma[i];
        if (current_centi_ma < 0)
        {
            current_sign = '-';
            abs_current_centi_ma = (uint32_t)(-current_centi_ma);
        }
        else
        {
            current_sign = '+';
            abs_current_centi_ma = (uint32_t)current_centi_ma;
        }

        if (voltage_mv < 0)
        {
            abs_voltage_mv = (uint32_t)(-voltage_mv);
        }
        else
        {
            abs_voltage_mv = (uint32_t)voltage_mv;
        }

        DebugUart_Printf(
            "[ADC] ADC_%u raw=%d V=%c%lu.%03lu V I=%c%lu.%02lu mA valid=%u\r\n",
            (unsigned int)adc420_channel_map[i].adc_id,
            (int)adc420_raw[i],
            voltage_sign,
            (unsigned long)(abs_voltage_mv / 1000UL),
            (unsigned long)(abs_voltage_mv % 1000UL),
            current_sign,
            (unsigned long)(abs_current_centi_ma / 100UL),
            (unsigned long)(abs_current_centi_ma % 100UL),
            (unsigned int)adc420_channel_valid[i]);
    }
}

uint8_t ADC420_SendCanFrames(void)
{
    CanTxMsg tx;
    uint8_t frame;
    uint8_t first_index;
    uint8_t all_ok;
    uint16_t can_id;

    if (adc420_scan_ok == 0U)
    {
        DebugUart_Printf("[CAN-TX] SKIP: ADC scan failed; no frames sent.\\r\\n");
        return 0U;
    }

    if (can_init_ok == 0U)
    {
        DebugUart_Printf("[CAN-TX] SKIP: CAN initialization failed; no frames sent.\\r\\n");
        return 0U;
    }

    all_ok = 1U;

    for (frame = 0U; frame < ADC420_CAN_FRAME_COUNT; frame++)
    {
        first_index = (uint8_t)(frame * 2U);
        can_id = (uint16_t)(ADC420_CAN_ID_BASE + frame);

        tx.StdId = can_id;
        tx.ExtId = 0U;
        tx.IDE = CAN_Id_Standard;
        tx.RTR = CAN_RTR_Data;
        tx.DLC = 8U;

        /* First channel: raw ADC counts, then current in 0.01 mA units. */
        ADC420_PackInt16LE(&tx.Data[0], adc420_raw[first_index]);
        ADC420_PackInt16LE(&tx.Data[2], adc420_current_centi_ma[first_index]);

        /* Second channel: raw ADC counts, then current in 0.01 mA units. */
        ADC420_PackInt16LE(&tx.Data[4], adc420_raw[first_index + 1U]);
        ADC420_PackInt16LE(&tx.Data[6], adc420_current_centi_ma[first_index + 1U]);

        adc420_last_can_id = can_id;
        {
            uint8_t tx_status;
            tx_status = CAN_SendMessage(&tx);

            if (tx_status == CAN_TxStatus_Ok)
            {
                adc420_last_can_status = 1U;
                adc420_can_frame_count++;
            }
            else
            {
                adc420_last_can_status = 0U;
                adc420_can_error_count++;
                all_ok = 0U;
            }

            DebugUart_Printf(
                "[CAN-TX] ID=0x%03X DLC=%u DATA=%02X %02X %02X %02X %02X %02X %02X %02X RESULT=%s(%u)\\r\\n",
                (unsigned int)can_id,
                (unsigned int)tx.DLC,
                (unsigned int)tx.Data[0],
                (unsigned int)tx.Data[1],
                (unsigned int)tx.Data[2],
                (unsigned int)tx.Data[3],
                (unsigned int)tx.Data[4],
                (unsigned int)tx.Data[5],
                (unsigned int)tx.Data[6],
                (unsigned int)tx.Data[7],
                (tx_status == CAN_TxStatus_Ok) ? "OK" : "FAIL",
                (unsigned int)tx_status);
        }
    }

    return all_ok;
}
