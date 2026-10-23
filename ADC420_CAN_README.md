# STM32F103C8T6: 10-channel 4-20 mA ADC acquisition over CAN

## Hardware and scan mapping

The three ADS1118 devices share SPI1. All chip selects are active-low and only one
chip is selected at a time.

- SPI1: PA5=SCK, PA6=MISO/DOUT, PA7=MOSI/DIN
- N_O1: PB0, channels AIN0..AIN3 -> ADC_4, ADC_5, ADC_7, ADC_6
- N_O2: PB1, channels AIN2..AIN3 -> ADC_2, ADC_3
- N_O3: PB10, channels AIN0..AIN3 -> ADC_11, ADC_9, ADC_8, ADC_10
- CAN1: PA12=TX, PA11=RX, 500 kbit/s (assuming PCLK1=36 MHz)

The 10 current inputs use 100 ohm shunts, so 4-20 mA corresponds to 0.4-2.0 V.
ADC PGA is +/-2.048 V. Voltage is calculated as raw * 0.0000625 V and current
as voltage * 10 mA/V. Readings are not clipped to 4-20 mA.

## CAN frame allocation

Five standard 8-byte data frames are sent after each successful full scan.
IDs are hexadecimal. All 16-bit fields are little-endian two's-complement values.

| CAN ID | First channel | Second channel |
|---|---|---|
| 0x301 | ADC_2 | ADC_3 |
| 0x302 | ADC_4 | ADC_5 |
| 0x303 | ADC_6 | ADC_7 |
| 0x304 | ADC_8 | ADC_9 |
| 0x305 | ADC_10 | ADC_11 |

Each frame uses the following layout:

- bytes 0-1: first channel raw ADC result (signed int16)
- bytes 2-3: first channel current in 0.01 mA units (signed int16)
- bytes 4-5: second channel raw ADC result (signed int16)
- bytes 6-7: second channel current in 0.01 mA units (signed int16)

Example: 4.00 mA is encoded as 400; 20.00 mA is encoded as 2000.
The raw result can be converted back to voltage using raw * 62.5 uV.

The main loop performs a complete scan and then waits 80 ms. Since conversion
time is about 2 ms per channel, the overall sample-and-send period is roughly
100 ms. A CAN frame is sent only if all 10 ADC reads succeeded.

## Keil Watch / debug variables

- ADC results: `adc420_raw[0..9]`, `adc420_voltage_v[0..9]`,
  `adc420_current_ma[0..9]`, `adc420_current_centi_ma[0..9]`
- Channel validity: `adc420_channel_valid[0..9]`, `adc420_scan_ok`
- Scan status: `adc420_scan_count`, `adc420_scan_fail_count`
- SPI status: `ads1118_spi_ready`, `ads1118_spi_timeout_count`,
  `ads1118_last_chip`, `ads1118_last_channel`, `ads1118_last_config`,
  `ads1118_last_raw`
- CAN status: `can_init_ok`, `can_last_tx_ok`, `can_tx_count`,
  `can_tx_fail_count`, `adc420_last_can_id`, `adc420_last_can_status`,
  `adc420_can_frame_count`, `adc420_can_error_count`

CAN must be connected through the board's CAN transceiver to a CAN adapter/host
configured for 500 kbit/s. Check the adapter's termination and common ground.
