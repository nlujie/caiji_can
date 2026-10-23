# STM32F103C8T6: 10-channel ADS1118 raw ADC data over CAN

## Hardware and ADC channel mapping

Three ADS1118 devices share SPI1. All chip-select signals are active-low and only
one ADS1118 is selected at a time.

- SPI1: PA5=SCK, PA6=MISO/DOUT, PA7=MOSI/DIN
- N_O1: PB0, AIN0..AIN3 -> ADC_4, ADC_5, ADC_7, ADC_6
- N_O2: PB1, AIN2..AIN3 -> ADC_2, ADC_3
- N_O3: PB10, AIN0..AIN3 -> ADC_11, ADC_9, ADC_8, ADC_10
- CAN1: PA12=TX, PA11=RX, 500 kbit/s when APB1=36 MHz
- UART debug: USART1 TX on PA9, 115200 baud, 8-N-1

The ten ADC channels continue to be scanned by the existing ADS1118 driver.
The UART diagnostic output still prints raw counts, voltage, current, channel
validity, CAN payload bytes and CAN transmit status. CAN payloads now contain
raw ADC counts only; current values are not sent over CAN.

## CAN message allocation

All messages are standard 11-bit CAN data frames. Each raw ADC sample occupies
exactly two bytes, encoded as a signed 16-bit ADS1118 result in little-endian
order (least-significant byte first).

| CAN ID | DLC | Byte offsets | Raw ADC channel order |
|---|---:|---|---|
| 0x110 | 8 | 0-1, 2-3, 4-5, 6-7 | ADC_10, ADC_9, ADC_8, ADC_11 |
| 0x111 | 8 | 0-1, 2-3, 4-5, 6-7 | ADC_5, ADC_4, ADC_7, ADC_6 |
| 0x112 | 4 | 0-1, 2-3 | ADC_3, ADC_2 |

Detailed byte layout:

- **0x110**
  - bytes 0-1: ADC_10 raw result (signed int16)
  - bytes 2-3: ADC_9 raw result (signed int16)
  - bytes 4-5: ADC_8 raw result (signed int16)
  - bytes 6-7: ADC_11 raw result (signed int16)
- **0x111**
  - bytes 0-1: ADC_5 raw result (signed int16)
  - bytes 2-3: ADC_4 raw result (signed int16)
  - bytes 4-5: ADC_7 raw result (signed int16)
  - bytes 6-7: ADC_6 raw result (signed int16)
- **0x112**
  - bytes 0-1: ADC_3 raw result (signed int16)
  - bytes 2-3: ADC_2 raw result (signed int16)
  - DLC is 4 because only two ADC values are transmitted.

The raw ADC count can be converted to input voltage using:
`V = raw * 0.0000625 V` for the current ADS1118 +/-2.048 V PGA setting.
For the 100 ohm shunt, the equivalent current is `I(mA) = V * 10`, but
these calculated values are **not included** in the CAN payload.

## Example

If the ordered raw values are ADC_10=21258, ADC_9=100, ADC_8=200, ADC_11=300,
then CAN ID 0x110 sends:

`DATA=0A 53 64 00 C8 00 2C 01`

CAN ID 0x112 sends four bytes only; bytes 4-7 may appear as zero in the local
UART debug print but are not part of that CAN frame because DLC=4.

## UART debug output

Connect PA9 to USB-UART RX and connect grounds together. Configure 115200 baud,
8 data bits, no parity, 1 stop bit. The UART prints raw ADC readings and each
CAN transmit attempt including ID, DLC, all eight local data-buffer bytes, and
TX status. For DLC=4, only the first four bytes are transmitted on CAN.

A CAN frame is sent only when all ten ADC SPI reads succeed and CAN initialization
succeeded.
