# 10-channel 4–20 mA ADS1118 to CAN (from earliest populated branch)

**Original base:** `main_can_v0_1019_1504` (first full STM32 firmware branch). The bare `main` initial commit only has a README. The original code and libraries remain in this branch for reference, but the `Project.uvprojx` target only builds the new acquisition application.

## PCB connectivity — checked against schematic + "二、采集部分"

- MCU STM32F103C8T6; SPI1 PA5=SCK, PA6=MISO, PA7=MOSI.
- ADCs: three ADS1118, all on SPI1, VDD=A3.3V, analog ground AGND. SPI lines traverse ISO7741F digital isolator.
- **Critical select polarity:** PB0->ISO7741F->N_O1, PB0 **LOW** selects; PB1->PC817 optical inverter->N_O2, PB1 **HIGH** selects; PB10->PC817 optical inverter->N_O3, PB10 **HIGH** selects. All deselected: PB0=1, PB1=0, PB10=0. Selecting N_O2/3 with a LOW as on the prior refactor will NOT work.
- Each current input has 100-ohm/0.1% shunt, transient protection, LMV324 unity-gain buffer and filter before ADS1118 AINx. 4-20mA -> 0.400-2.000V before gain (nominally unity). ADS1118 PGA +/-2.048V, 16-bit signed, SPI mode 1, single-shot, 860SPS, 32-clock config+echo, 3ms conversion wait then 16-clock result read.
- Do NOT include PT100 (P12/P13), actuator feedback (ADC_0/1), leak inputs, PWM and Modbus in this target.

| Array index | Logical input | Socket | ADS1118 chip | AIN | MCU chip select |
|---:|---|---|---|---|---|
| 0 | ADC_2 | P10 | N_O2 | 2 | PB1 HIGH |
| 1 | ADC_3 | P11 | N_O2 | 3 | PB1 HIGH |
| 2 | ADC_4 | P1 | N_O1 | 0 | PB0 LOW |
| 3 | ADC_5 | P2 | N_O1 | 1 | PB0 LOW |
| 4 | ADC_6 | P4 | N_O1 | 3 | PB0 LOW |
| 5 | ADC_7 | P5 | N_O1 | 2 | PB0 LOW |
| 6 | ADC_8 | P9 | N_O3 | 2 | PB10 HIGH |
| 7 | ADC_9 | P7 | N_O3 | 1 | PB10 HIGH |
| 8 | ADC_10 | P8 | N_O3 | 3 | PB10 HIGH |
| 9 | ADC_11 | P6 | N_O3 | 0 | PB10 HIGH |

The manual repeats P9 pin numbers "11/12" in the P6 section. Inspect the physical P6 connector/pin 1 when connecting; do not blindly follow the duplicated numbers.

## CAN protocol

- CAN1 PA11 RX, PA12 TX; connector **U25 pin1 CAN_H, pin2 CAN_L**.
- **500 kbit/s**, standard 11-bit identifier, normal mode, 100-ms scan period.
- Each current value is **uint16 little endian in microamps** (uA); e.g. 12.000mA = 12000 = 0x2EE0 -> bytes `E0 2E`.
- Invalid samples (SPI timeout/config echo mismatch/negative ADC code) use `0xFFFF` and valid bit cleared. Fault bit also sets when value outside 3.6–21 mA, but SPI success itself does not mean sensor signal is correct.
- `0x310` DLC8 = ADC_2, ADC_3, ADC_4, ADC_5.
- `0x311` DLC8 = ADC_6, ADC_7, ADC_8, ADC_9.
- `0x312` DLC4 = ADC_10, ADC_11.
- `0x313` DLC8 = LE u16 valid bitmask, LE u16 fault bitmask, byte4 scan sequence, byte5 CAN failures for 3 data frames, byte6 protocol version 1, byte7 reserved.
- Bit0 in both bitmasks refers to ADC_2; bit9 refers to ADC_11.

## Keil 5 / ARMCC5 bringup

Open `Project.uvprojx`, **Rebuild All**, flash to STM32F103C8.
Only new `adc420_spi.c`, `adc420_app.c`, `adc420_can.c`, `User/main.c` and `stm32f10x_it.c` are compiled at application level, along with the startup and STM32 peripheral library modules. All file paths use single Windows separators: `.\Hardware\...`.

In View -> Watch 1, add:
`adc420_ms`, `adc420_scan_count`, `adc420_sequence`,
`adc420_raw[2]`, `adc420_current_ua[2]`, `adc420_valid_mask`,
`adc420_fault_mask`, `adc420_spi_state[0]`,
`adc420_spi_cfg_sent[0]`, `adc420_spi_cfg_echo[0]`,
`adc420_spi_previous_result[0]`, `adc420_spi_success_count[0]`,
`adc420_can_tx_ok`, `adc420_can_tx_failed`.

**P1 / ADC_4:** index **2**, N_O1/PB0/AIN0. 4mA->raw about 6400/current 4000uA; 12mA->19200/12000uA; 20mA->32000/20000uA. Meter the 100-ohm shunt relative to AGND: 0.4/1.2/2.0V. Only then inspect LMV324 output/ADS1118 AIN0. The P1 second contact is 24V output, **never short it directly to the ADC input**.

`adc420_spi_state[0]` codes: 0 not measured, 1 SPI+config echo passed, 2 SPI transfer timeout, 3 config echo mismatch. Expect `adc420_spi_cfg_echo[0] & 0x7FFE == adc420_spi_cfg_sent[0] & 0x7FFE`. Successful serial echo is necessary but not sufficient to prove the analog channel is wired correctly.

If `adc420_scan_count` increases but `adc420_raw[2]` is fixed, use controlled stimulus changes, check actual current loop and sensor output before blaming CAN. Confirm AGND analog reference continuity, A3.3V ADC supply, power for the LMV324. If `adc420_spi_state[0] == 3`, scope CS at ADS1118 side **after** isolation and check shared SCLK/MISO/MOSI. Test PB1/PB10 with inverted select rule.

A **real** CAN analyzer/other active node is required for ACK; no ACK causes CAN transmit failures and is not evidence ADC acquisition failed. Install suitable 120-ohm termination at bus ends.

## Validation

This change was reconstructed from source schematics and TI ADS1118 Rev F register/protocol documentation. Git object/path checks and arithmetic/protocol logic tests were performed, **not** a live Keil ARMCC5 build or physical board/CAN bus test. A bench test is still required.
