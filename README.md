# main_ADC_1023_1218 — 10-channel 4–20 mA acquisition / CAN

Standalone ADC acquisition target based on `main_CAN_1022_1411`. Open `Project.uvprojx` using Keil uVision / ARMCC 5.06u5. This target **only** compiles the new `adc420_*` modules; legacy PT100, PWM, Modbus and the original ADS1118 driver remain in the repository for reference but are not part of this build.

## Hardware and mapping

ADC IC: three ADS1118, all on **SPI1 PA5=SCK, PA6=MISO, PA7=MOSI**; software chip selects **N_O1=PB0, N_O2=PB1, N_O3=PB10**. Bus uses SPI mode 1 and 2.25 MHz assuming a 72-MHz CPU. All current sensors have a **100 ohm 0.1% shunt**, giving 4 mA → 0.400 V, 12 mA → 1.200 V, 20 mA → 2.000 V. ADS1118 PGA: +/-2.048 V; 860 SPS, single-shot, 3 ms/channel settle. The ADC conversion is **not** STM32's internal ADC.

| Sensor | Connector | ADC IC / CS | IC input |
|:--|:--|:--|:--|
| ADC_2 | P10 pin 4 | N_O2 / PB1 | AIN2 |
| ADC_3 | P11 pin 1 | N_O2 / PB1 | AIN3 |
| ADC_4 | P1 pin 3 | N_O1 / PB0 | AIN0 |
| ADC_5 | P2 pin 1 | N_O1 / PB0 | AIN1 |
| ADC_6 | P4 pin 7 | N_O1 / PB0 | AIN3 |
| ADC_7 | P5 pin 5 | N_O1 / PB0 | AIN2 |
| ADC_8 | P9 pin 11 | N_O3 / PB10 | AIN2 |
| ADC_9 | P7 pin 9 | N_O3 / PB10 | AIN1 |
| ADC_10 | P8 pin 7 | N_O3 / PB10 | AIN3 |
| ADC_11 | P6 (physical pin 1) | N_O3 / PB10 | AIN0 |

**P6 physical pin-number note:** the description repeats “11/12” (already used for P9) despite the connector image using that position. Verify the board's P6 labeling before attaching a sensor.

The four ADS1118 channels not in the 10-input mapping (N_O2 AIN0/1 and any unused inputs) are excluded by design. PT100/PT1000 excitation, actuator feedback and other control features are outside this acquisition-only target.

## CAN format

- Port: **U25 CAN_H pin1, CAN_L pin2**; STM32 **PA12 TX, PA11 RX**, no remap.
- CAN 2.0A **standard 11-bit IDs, data frames, 500,000 bps**; cycle nominally 100 ms.
- Each sensor is transmitted as **unsigned integer microamps (uA)**: 4 mA = 4000 (`0x0FA0`), 12 mA = 12000 (`0x2EE0`), 20 mA = 20000 (`0x4E20`). Words are **little-endian**. Invalid readings are `0xFFFF`, and are marked invalid in the status frame.
- `0x310`, DLC 8: ADC_2 / ADC_3 / ADC_4 / ADC_5 (four consecutive LE u16).
- `0x311`, DLC 8: ADC_6 / ADC_7 / ADC_8 / ADC_9.
- `0x312`, DLC 4: ADC_10 / ADC_11.
- `0x313`, DLC 8: byte0–1 valid bitmask; byte2–3 fault bitmask; byte4 sequence (increments once per cycle); byte5 failed **data** CAN frames this cycle; byte6 acquisition + 3-frame transmission elapsed ms saturated to 255; byte7 protocol version (`1`). Bit 0 in both masks means ADC_2, bit 9 means ADC_11.
- Fault = SPI error / negative raw code / current <3.6mA or >21mA; invalid is only for read errors/negative ADC raw. Under- and over-range measurements still carry their measured value and remain valid.
- SPI cannot reliably detect some physical disconnections (floating MISO). A believable but incorrect value requires circuit-level or calibrated-current testing.

## Firmware architecture

`User/main.c`: 1-ms SysTick, 100-ms cooperative schedule.

`Hardware/adc420_ads1118.[ch]`: GPIO CS + SPI1 mode 1; new single-shot conversion and 16-bit retrieval for each channel; timeouts.

`Hardware/adc420_app.[ch]`: single source-of-truth 10-channel map, conversion, masks, packing.

`Hardware/adc420_can.[ch]`: 500kbps normal-mode CAN1 + bounded transmit check. **Real CAN requires an external receiver/ACK and a powered transceiver; internal loopback does not prove bus operation.**

## Bring-up / debugger checkpoints

1. Keil Rebuild All, flash MCU; inspect `adc420_tick_ms` increments every 1ms.
2. Check PB0/PB1/PB10 idle HIGH, only the selected chip's CS goes LOW while SPI clocks PA5.
3. Inject 4.000 mA at one channel at a time using a calibrated source, respecting sensor wiring, 24V and ground polarity. Probe across the 100-ohm input shunt: should be 0.400V. In Keil Watch, inspect `adc420_raw[index]` ~6400, `adc420_current_ua[index]` ~4000. Repeat 12mA (~19200, 12000uA), 20mA (~32000, 20000uA).
4. Connect U25 CAN_H/CAN_L to a 500kbps CAN adapter with shared reference and correct bus termination; observe IDs 0x310..0x313. 10 slots must update in each ~100-ms group. Test every sensor channel independently against mapping table.
5. Disconnect a current input: fault mask should normally set if current drops below 3.6mA. Disconnect/short SPI and observe `adc420_valid_mask`, `adc420_fault_mask`; do not assume a floating bus always generates a detectable SPI timeout.
6. Verify reset, uninterrupted cycles, CAN unplug/replug and CAN error recovery. If CAN init fails, `can_ready` remains zero until restart; CAN-bus errors after successful init can recover through bxCAN auto bus-off management.

**Validation status**: source/schema and conversion arithmetic are reviewable; no ARMCC hardware build, SWD flash, oscilloscope or physical CAN measurement can be performed in this session. Rebuild All and bench qualification are required before deployment.

**Sources**: board schematic `stm32f103c8t6(1).pdf`, `使用说明(1).docx` section 二; TI ADS1118 datasheet (SBAS457F).


## Keil Watch diagnostics (P1 connected but `adc420_raw[2]` is zero)

`adc420_raw[2]` = P1 / ADC_4 (ADS1118 N_O1/PB0 AIN0).
The new driver clocks a complete **32-bit write + config readback** before
the 16-bit measurement read. It rejects unacknowledged configuration rather
than incorrectly marking floating SPI as valid.

Watch these global variables, using Run / Halt cycles (or enable Periodic Window Update):

- `adc420_tick_ms`: 1-ms timer, should increase.
- `adc420_sequence`: ~10 increments/s, wraps after 255.
- `adc420_diag_temp_ok_mask`: startup temperature self-test; bit0 means N_O1 responding, bit1 N_O2, bit2 N_O3; typically `0x07`.
- `adc420_diag_temp_raw[0]`: N_O1 internal temperature 16-bit left-aligned reading. Near 25C -> ~3200. `-32768` means read failed. **A valid internal temperature is independent of P1 sensor wiring.**
- `adc420_diag_spi_state[0]`: N_O1 digital-link status: 0 not read, 1 verified, 2 SPI timeout, 3 config echo mismatch, 4 conversion read timeout.
- `adc420_diag_config_written[0]`: most recent cfg sent to N_O1 (cycling between AIN0..3).
- `adc420_diag_config_echo[0]`: N_O1 response to same cfg; bits 14:4 must match the request.
- `adc420_diag_read_count[0]`: successful N_O1 reads, increases 4 times per acquisition cycle.
- `adc420_raw[2]`: ADC_4, about 6400 at 4mA / 19200 at 12mA / 32000 at 20mA.
- `adc420_current_ua[2]`: approx 4000 / 12000 / 20000 uA.

**Interpreting the diagnostic**:

1. If timer/sequence stay fixed across Run→Halt cycles, check the program's state/debugger.
2. If `adc420_diag_spi_state[0] = 3` or `adc420_diag_temp_ok_mask & 1 == 0`, inspect N_O1 PB0, PA5/PA6/PA7, ADS1118 supply and board routing. Normal SPI controller transfers alone did not guarantee a connected ADS1118.
3. If N_O1 self-test passes but `adc420_raw[2]` stays ~0, use a multimeter to check the **100-ohm shunt** across P1's current-input path and the conditioned AIN0 node. Expected shunt voltage 0.4..2.0V. P1 is a connector for a live current loop, not a source of current. Confirm 24V supply and correct return path before injecting a calibrated 4/12/20mA.
4. If shunt is 0.4..2.0V but AIN0 is near 0, troubleshoot LMV324 analog buffer / filtering / shorts. If AIN0 has valid voltage but ADS1118 raw remains zero, inspect SPI configuration and chip input soldering.
5. CAN failures (e.g., `adc420_can_error_count=4` if no ACK) are independent of the ADC analog path and must not be confused with sampling errors.

**IMPORTANT:** Self-test and config readback improve diagnostics; they do not replace an ARMCC build or bench validation. Existing CAN frame protocol and channel mapping remain unchanged.
