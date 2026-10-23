# STM32F103C8T6: 10-channel ADS1118 acquisition and engineering CAN data

## Hardware and ADC channel mapping

Three ADS1118 devices share SPI1. All chip-select signals are active-low and only
one ADS1118 is selected at a time.

- SPI1: PA5=SCK, PA6=MISO/DOUT, PA7=MOSI/DIN
- N_O1: PB0, AIN0..AIN3 -> ADC_4, ADC_5, ADC_7, ADC_6
- N_O2: PB1, AIN2..AIN3 -> ADC_2, ADC_3
- N_O3: PB10, AIN0..AIN3 -> ADC_11, ADC_9, ADC_8, ADC_10
- CAN1: PA12=TX, PA11=RX, 500 kbit/s when APB1=36 MHz
- UART debug: USART1 TX on PA9, 115200 baud, 8-N-1

All current inputs use a 100 ohm shunt. With the ADS1118 PGA set to +/-2.048 V,
the nominal 4-20 mA input corresponds to 0.4-2.0 V and raw counts of about
6400-32000.

## Engineering conversions

### Temperature

Temperature sensor order is ADC_10, ADC_9, ADC_8, ADC_11.

- 4 mA -> -40.00 C
- 20 mA -> +80.00 C
- Linear mapping: T(C) = -40 + (I(mA) - 4) * 120 / 16
- CAN encoding: signed int16, little-endian, unit 0.01 C
- The output is clamped to -40.00..+80.00 C for raw ADC counts outside the
  nominal 4-20 mA range.

### Pressure

Pressure sensor order is ADC_5, ADC_4, ADC_7, ADC_6.

- 4 mA -> 0.00 bar
- 20 mA -> 10.00 bar
- Linear mapping: P(bar) = (I(mA) - 4) * 10 / 16
- CAN encoding: uint16, little-endian, unit 0.01 bar
- The output is clamped to 0.00..10.00 bar for raw ADC counts outside the
  nominal 4-20 mA range.

### Flow

Flow signal order is ADC_3 followed by ADC_2. Flow data is sent directly as
the raw signed 16-bit ADS1118 conversion count, little-endian. No flow
engineering-unit conversion is performed.

## CAN message allocation

All messages are standard 11-bit CAN data frames. Each engineering value occupies
two bytes, little-endian.

| CAN ID | DLC | Byte offsets | Data |
|---|---:|---|---|
| 0x110 | 8 | 0-1 | TEMP1: ADC_10, signed int16, 0.01 C |
| 0x110 | 8 | 2-3 | TEMP2: ADC_9, signed int16, 0.01 C |
| 0x110 | 8 | 4-5 | TEMP3: ADC_8, signed int16, 0.01 C |
| 0x110 | 8 | 6-7 | TEMP4: ADC_11, signed int16, 0.01 C |
| 0x111 | 8 | 0-1 | PRESS1: ADC_5, uint16, 0.01 bar |
| 0x111 | 8 | 2-3 | PRESS2: ADC_4, uint16, 0.01 bar |
| 0x111 | 8 | 4-5 | PRESS3: ADC_7, uint16, 0.01 bar |
| 0x111 | 8 | 6-7 | PRESS4: ADC_6, uint16, 0.01 bar |
| 0x112 | 4 | 0-1 | FLOW1: ADC_3 raw count, signed int16 |
| 0x112 | 4 | 2-3 | FLOW2: ADC_2 raw count, signed int16 |

Bytes are little-endian: the low byte is transmitted first. For example, a
temperature of -12.34 C is encoded as signed value -1234, two's-complement
0xFB2E, transmitted as `2E FB`. A temperature of +25.67 C is 2567 (0x0A07),
transmitted as `07 0A`. A pressure of 7.50 bar is 750 (0x02EE), transmitted
as `EE 02`.

For flow, decode the two bytes as a signed 16-bit raw ADC count. It is not mA,
flow rate, or a calibrated engineering value.

## UART debug output

Connect PA9 to USB-UART RX and connect grounds together. Configure 115200 baud,
8 data bits, no parity, 1 stop bit. The UART prints raw ADC readings, converted
temperature and pressure values, CAN IDs, DLC, all eight displayed data bytes,
and TX status. For 0x112, DLC is 4 and bytes 4-7 are zero-filled in the debug
line but are not transmitted.

A CAN frame is sent only when all ten ADC SPI reads succeed and CAN initialization
succeeded. CAN TX status OK means the controller completed a successful CAN
transmission; it does not by itself verify application-level decoding by the host.
