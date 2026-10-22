# ADS1118 ADC → CAN protocol (main_ADC_SPI_1022_1607)

## Channel confirmation

The existing firmware uses **3 × ADS1118 × 4 single-ended AIN inputs = 12 ADC inputs**. Only **7 inputs** are currently configured as 4–20 mA; the other 5 are **4 thermistor** inputs and **1 voltage** input. These are firmware declarations, not a guarantee of PCB wiring.

| Current channel | ADC | Input | Existing logical use |
|---|---|---|---|
| CH0 | SPI1 / CS PA4 | AIN0 | Flow 1 |
| CH1 | SPI1 / CS PA4 | AIN1 | Flow 2 |
| CH2 | SPI1 / CS PA3 | AIN0 | Pressure 1 |
| CH3 | SPI1 / CS PA3 | AIN1 | Pressure 2 |
| CH4 | SPI1 / CS PA3 | AIN2 | Pressure 3 |
| CH5 | SPI2 / CS PB12 | AIN1 | Pressure 4 |
| CH6 | SPI2 / CS PB12 | AIN2 | Pressure 5 |

Remaining ADC inputs: SPI1 PA4 AIN2/3 and SPI1 PA3 AIN3 and SPI2 PB12 AIN3 (thermistors); SPI2 PB12 AIN0 (0–10 V).

**Important:** Some old comments say SPI1 chip 2 CS=PA8, but the actual GPIO macro and initializer select **PA3**. Confirm against the board schematic. Existing CAN init uses **PA11 RX / PA12 TX**, not the PB8/PB9 remap. Verify the transceiver pin connections. Hardware ADC inputs do not connect to CAN directly; MCU reads ADS1118 using SPI and sends processed values over CAN.

## CAN wire format

500 kbit/s (configured with 36 MHz PCLK1, 1+9+8 TQ, prescaler 4); **11-bit standard ID**, data frames, DLC 8, period nominal **200 ms**, updated immediately after `Read_ADS1118_Direct()`. Channel words are **unsigned 16-bit little endian, unit 0.01 mA (value / 100)**.

| Standard ID | Bytes 0-1 | 2-3 | 4-5 | 6 | 7 |
|---|---|---|---|---|---|
| 0x120 | CH0 u16 | CH1 u16 | CH2 u16 | CH3 low byte | CH3 high byte |
| 0x121 | CH4 u16 | CH5 u16 | CH6 u16 | validity mask | sequence counter |

In ID `0x120`, bytes **6–7 together** form the CH3 u16 value. Each current word is `ADC_code / 16` rounded (ADC1118 voltage at 0.0625 mV/code across a 100-ohm shunt). Hence **4.00 mA = 400 = 0x0190**, **12.00 mA = 1200 = 0x04B0**, **20.00 mA = 2000 = 0x07D0**.

Valid range follows the existing 4–20 mA ADC guard: inclusive ADC codes 6400–32000. **`0xFFFF` means invalid, unconnected, or out-of-range**; do not treat it as a current. ID 0x121 byte 6: bit `n` set means CH`n` valid (bits 7 reserved 0). Byte 7 is an 8-bit modulo-256 sequence number for each acquisition cycle. For complete samples, ensure both frames are fresh; reject stale frames on the receiver. As the sequence is only on 0x121, proximity in receive time is also required to pair 0x120 with 0x121.

Original CAN payloads are **unchanged** and still sent:
- `0x110`: 4 filtered ADC raw 16-bit counts for SPI1 / PA4 AIN0–3 (8 bytes)
- `0x111`: 4 filtered ADC raw counts for SPI1 / PA3 AIN0–3
- `0x112`: 4 filtered ADC raw counts for SPI2 / PB12 AIN0–3

The raw frames also use little-endian u16, so all 12 ADC measurements remain available for temperature and voltage calculations on the receiver.

## Example CAN analyzer test

1. Connect CAN transceivers (CAN_H/CAN_L and appropriate common reference) and terminate **120 ohms at each physical bus end**. Use a 500-kbit/s CAN analyzer in normal/ACK-capable mode.
2. Power the ADC board; observe IDs **0x110, 0x111, 0x112, 0x120, 0x121**. Each ID should appear around once per 200 ms; serial diagnostics and polling may introduce jitter.
3. Apply a calibrated 4 mA, 12 mA, and 20 mA input independently to each of CH0–CH6; expect approximately **400**, **1200**, **2000** on its CAN slot. Verify byte order (e.g. 12 mA -> **B0 04**).
4. Disconnect a loop; corresponding current should become `FF FF` after filter settling, and its corresponding valid bit should clear.
5. Watch `can_adc_current_valid_mask`, `can_adc_current_sample_sequence`, and `can_adc_current_tx_busy_count` in Keil for diagnosis. Incrementing busy count means a frame could not obtain a CAN transmit mailbox.
6. Verify readings on the real board. This commit is code-reviewed but **not Keil-built or hardware-tested**.

## Engineering notes

- New 0x120/0x121 frames preserve old 0x110–0x112 consumers. The change also corrects two AIN3 copy/paste errors, configures both MISO pins as inputs, and enables CAN automatic retransmission/bus-off recovery.
- There are three ADS1118 devices. Firmware channels are not MCU internal ADC channels.
- Existing ADS1118 conversion/filter routines and calibration factors are otherwise unchanged; confirm board input scaling, reference, shunts, and chip select wiring.
