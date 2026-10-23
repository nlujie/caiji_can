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

/* Engineering channel order requested for CAN upload. */
static const uint8_t adc420_temperature_adc_ids[ADC420_TEMP_COUNT] =
{
    10U, 9U, 8U, 11U
};

static const uint8_t adc420_pressure_adc_ids[ADC420_PRESSURE_COUNT] =
{
    5U, 4U, 7U, 6U
};

volatile int16_t adc420_raw[ADC420_CHANNEL_COUNT] = { 0 };
volatile float adc420_voltage_v[ADC420_CHANNEL_COUNT] = { 0.0f };
volatile float adc420_current_ma[ADC420_CHANNEL_COUNT] = { 0.0f };
volatile int16_t adc420_current_centi_ma[ADC420_CHANNEL_COUNT] = { 0 };
volatile uint8_t adc420_channel_valid[ADC420_CHANNEL_COUNT] = { 0U };

volatile int16_t adc420_temperature_centi_c[ADC420_TEMP_COUNT] = { 0 };
volatile uint16_t adc420_pressure_centi_bar[ADC420_PRESSURE_COUNT] = { 0U };

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

static void ADC420_PackUint16LE(uint8_t *dst, uint16_t value)
{
    dst[0] = (uint8_t)(value & 0x00FFU);
    dst[1] = (uint8_t)(value >> 8);
}

static void ADC420_PackInt16LE(uint8_t *dst, int16_t value)
{
    ADC420_PackUint16LE(dst, (uint16_t)value);
}

static int16_t ADC420_ConvertTemperatureCentiC(int16_t raw)
{
    int32_t bounded_raw;
    int32_t delta_raw;
    int32_t temperature_centi_c;

    /*
     * ADS1118 PGA +/-2.048 V: 1 count = 62.5 uV.
     * 100 ohm shunt: 4 mA -> 0.4 V -> raw about 6400.
     *                  20 mA -> 2.0 V -> raw about 32000.
     * Linear conversion: -40..80 C, stored as 0.01 C.
     * Clamp to the specified 4..20 mA engineering range.
     */
    bounded_raw = (int32_t)raw;
    if (bounded_raw < 6400L)
    {
        bounded_raw = 6400L;
    }
    else if (bounded_raw > 32000L)
    {
        bounded_raw = 32000L;
    }

    delta_raw = bounded_raw - 6400L;
    /* 12000 centi-degrees / 25600 ADC counts = 15/32. Round nearest. */
    temperature_centi_c = -4000L + ((delta_raw * 15L + 16L) / 32L);

    return (int16_t)temperature_centi_c;
}

static uint16_t ADC420_ConvertPressureCentiBar(int16_t raw)
{
    int32_t bounded_raw;
    int32_t delta_raw;
    int32_t pressure_centi_bar;

    /*
     * 4 mA -> 0 bar, 20 mA -> 10 bar.
     * Stored as 0.01 bar. Clamp to the specified 4..20 mA range.
     * 1000 centi-bar / 25600 ADC counts = 5/128.
     */
    bounded_raw = (int32_t)raw;
    if (bounded_raw < 6400L)
    {
        bounded_raw = 6400L;
    }
    else if (bounded_raw > 32000L)
    {
        bounded_raw = 32000L;
    }

    delta_raw = bounded_raw - 6400L;
    pressure_centi_bar = (delta_raw * 5L + 64L) / 128L;

    return (uint16_t)pressure_centi_bar;
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

    for (i = 0U; i < ADC420_TEMP_COUNT; i++)
    {
        adc420_temperature_centi_c[i] = 0;
    }

    for (i = 0U; i < ADC420_PRESSURE_COUNT; i++)
    {
        adc420_pressure_centi_bar[i] = 0U;
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
    uint8_t adc_index;

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
         */
        adc420_voltage_v[i] = (float)raw * 0.0000625f;
        adc420_current_ma[i] = adc420_voltage_v[i] * 10.0f;
        adc420_current_centi_ma[i] = (int16_t)(raw / 16);
        adc420_channel_valid[i] = 1U;
    }

    /* Convert sensor channels to engineering units after completing the scan. */
    for (i = 0U; i < ADC420_TEMP_COUNT; i++)
    {
        adc_index = (uint8_t)(adc420_temperature_adc_ids[i] - 2U);
        adc420_temperature_centi_c[i] =
            ADC420_ConvertTemperatureCentiC(adc420_raw[adc_index]);
    }

    for (i = 0U; i < ADC420_PRESSURE_COUNT; i++)
    {
        adc_index = (uint8_t)(adc420_pressure_adc_ids[i] - 2U);
        adc420_pressure_centi_bar[i] =
            ADC420_ConvertPressureCentiBar(adc420_raw[adc_index]);
    }

    adc420_scan_ok = all_ok;
    adc420_scan_count++;
    return all_ok;
}

/*
 * Print raw ADC/current readings plus the engineering values used by CAN.
 * Fixed-point output avoids requiring %f support in Keil printf.
 */
void ADC420_DebugPrintReadings(void)
{
    uint8_t i;
    int32_t voltage_mv;
    int32_t current_centi_ma;
    int32_t engineering_value;
    uint32_t abs_voltage_mv;
    uint32_t abs_current_centi_ma;
    uint32_t abs_engineering_value;
    char voltage_sign;
    char current_sign;
    char engineering_sign;

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

    for (i = 0U; i < ADC420_TEMP_COUNT; i++)
    {
        engineering_value = (int32_t)adc420_temperature_centi_c[i];
        if (engineering_value < 0)
        {
            engineering_sign = '-';
            abs_engineering_value = (uint32_t)(-engineering_value);
        }
        else
        {
            engineering_sign = '+';
            abs_engineering_value = (uint32_t)engineering_value;
        }

        DebugUart_Printf("[ENG] TEMP%u ADC_%u=%c%lu.%02lu C\r\n",
                         (unsigned int)(i + 1U),
                         (unsigned int)adc420_temperature_adc_ids[i],
                         engineering_sign,
                         (unsigned long)(abs_engineering_value / 100UL),
                         (unsigned long)(abs_engineering_value % 100UL));
    }

    for (i = 0U; i < ADC420_PRESSURE_COUNT; i++)
    {
        DebugUart_Printf("[ENG] PRESS%u ADC_%u=%u.%02u bar\r\n",
                         (unsigned int)(i + 1U),
                         (unsigned int)adc420_pressure_adc_ids[i],
                         (unsigned int)(adc420_pressure_centi_bar[i] / 100U),
                         (unsigned int)(adc420_pressure_centi_bar[i] % 100U));
    }

    DebugUart_Printf("[ENG] FLOW1 ADC_3 raw=%d, FLOW2 ADC_2 raw=%d\r\n",
                     (int)adc420_raw[1],
                     (int)adc420_raw[0]);
}

uint8_t ADC420_SendCanFrames(void)
{
    CanTxMsg tx;
    uint8_t frame;
    uint8_t i;
    uint8_t all_ok;
    uint8_t tx_status;
    uint16_t can_id;

    if (adc420_scan_ok == 0U)
    {
        DebugUart_Printf("[CAN-TX] SKIP: ADC scan failed; no frames sent.\r\n");
        return 0U;
    }

    if (can_init_ok == 0U)
    {
        DebugUart_Printf("[CAN-TX] SKIP: CAN initialization failed; no frames sent.\r\n");
        return 0U;
    }

    all_ok = 1U;

    for (frame = 0U; frame < ADC420_CAN_FRAME_COUNT; frame++)
    {
        can_id = (uint16_t)(ADC420_CAN_ID_BASE + frame);

        tx.StdId = can_id;
        tx.ExtId = 0U;
        tx.IDE = CAN_Id_Standard;
        tx.RTR = CAN_RTR_Data;
        tx.DLC = (frame < 2U) ? 8U : 4U;

        /* Clear all bytes so the UART debug output is deterministic. */
        for (i = 0U; i < 8U; i++)
        {
            tx.Data[i] = 0U;
        }

        if (frame == 0U)
        {
            /* 0x110: TEMP1..TEMP4, signed int16 in 0.01 degree C. */
            for (i = 0U; i < ADC420_TEMP_COUNT; i++)
            {
                ADC420_PackInt16LE(&tx.Data[(uint8_t)(i * 2U)],
                                   adc420_temperature_centi_c[i]);
            }
        }
        else if (frame == 1U)
        {
            /* 0x111: PRESS1..PRESS4, uint16 in 0.01 bar. */
            for (i = 0U; i < ADC420_PRESSURE_COUNT; i++)
            {
                ADC420_PackUint16LE(&tx.Data[(uint8_t)(i * 2U)],
                                    adc420_pressure_centi_bar[i]);
            }
        }
        else
        {
            /*
             * 0x112: raw ADC values only; each is a signed 16-bit ADS1118
             * conversion result. Required order: ADC_3 first, then ADC_2.
             */
            ADC420_PackInt16LE(&tx.Data[0], adc420_raw[1]);
            ADC420_PackInt16LE(&tx.Data[2], adc420_raw[0]);
        }

        adc420_last_can_id = can_id;
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
            "[CAN-TX] ID=0x%03X DLC=%u DATA=%02X %02X %02X %02X %02X %02X %02X %02X RESULT=%s(%u)\r\n",
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

    return all_ok;
}
