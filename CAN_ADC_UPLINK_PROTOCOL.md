# CAN ADC telemetry uplink (integration draft)

Based on `test/can1-internal-loopback-20261019`, which passed 100/100 *internal* frames on the board. This branch restores the original ADC collection application, replaces the diagnostic-only entry point, and uses CAN1 **normal** bus mode.

**The application has NOT been compiled on the user's Keil installation or validated on an external CAN bus yet.** Controller application framing requirements have not been provided; the IDs below are inherited from `main_can_v0_1019_1504` and are therefore **provisional** until the controller implements or approves them.

## Physical interface

- STM32F103C8T6 CAN1 PA12 TX / PA11 RX, through the board's external isolated transceiver to CANH/CANL.
- Classical CAN 2.0 standard 11-bit identifiers, data frames (RTR=0), bitrate **500 kbit/s**.
- 18 time quanta/bit: SJW=1, BS1=15, BS2=2, prescaler computed using actual APB1 clock (typically PCLK1=36 MHz, prescaler=4? **No**: 36 MHz/(500 kHz*18)=**4**, yielding exactly 500 kbit/s).
- Requires a powered external transceiver and at least one OTHER active matching CAN node to provide an ACK. CANH/CANL polarity and termination must be correct (normally 120 ohms at both physical ends of the bus).

## Telemetry protocol proposal (preserves existing IDs)

Every completed ADC acquisition queues one coherent set of three messages if the previous set is not being transmitted. A 200 ms scheduling interval is configured in `User/main.c`; actual interval may be longer if sensor reads or serial logging block. A new sample is skipped rather than overwriting an in-flight set.

| Standard ID | DLC | Origin | Byte order and data |
| --- | --- | --- | --- |
| `0x110` | 8 | ADS1118 SPI1 chip1 | AIN0, AIN1, AIN2, AIN3 |
| `0x111` | 8 | ADS1118 SPI1 chip2 | AIN0, AIN1, AIN2, AIN3 |
| `0x112` | 8 | ADS1118 SPI2 chip | AIN0, AIN1, AIN2, AIN3 |

Each AIN is a 16-bit **unsigned filtered ADC raw count**, **little-endian**:
- Byte0 = AIN0 low; Byte1 = AIN0 high.
- Byte2 = AIN1 low; Byte3 = AIN1 high.
- Byte4 = AIN2 low; Byte5 = AIN2 high.
- Byte6 = AIN3 low; Byte7 = AIN3 high.

Example: AIN0=0x1234, AIN1=0x5678, AIN2=0x9ABC, AIN3=0xDEF0 -> payload `34 12 78 56 BC 9A F0 DE`. All twelve channels retain the existing per-channel meanings defined in `Hardware/func.c`; these are **not floating point** values. Convert to engineering units only after agreeing on calibration, scaling, validation/status, and controller-side behavior.

## Firmware details and previous defects addressed

- Keeps the original ADC sampling, filter initialization, Modbus/serial setup and timer ISR.
- Removes the `CAN_SelfTest_Run()` / `while(1)` diagnostic-only halt from `main.c`.
- Replaces old immediate-transmit-status check with nonblocking TX mailbox polling; `CAN_TxStatus_Pending` is not considered failure.
- CAN hardware enables automatic retransmission and ABOM; stale pending TX requests are canceled after 30 ms to prevent indefinite backlog if nobody ACKs. Failure is counted and the next frame can be tried. A timeout does NOT imply frame was never observed on the physical bus.
- Avoids the old duplicate sends at the end of `Read_ADS1118_Direct()` and again in `task_200ms()`.
- Fixes the legacy filtered SPI1 chip1 AIN3 assignment and chip2 AIN3 raw assignment copy/paste errors.
- Preserves existing RX FIFO0 interrupt and callback for future controller messages. No controller commands are acted on in this branch; received messages are only made available in the legacy buffer.

## Keil Watch

- `can_uplink_init_ok` must be `1` (configured).
- `can_uplink_pclk1`, `can_uplink_prescaler`: effective timing.
- `can_uplink_frames_queued`: frames added to outgoing batches.
- `can_uplink_tx_ok`: **successfully ACKed by a CAN node**.
- `can_uplink_tx_fail`: failed/aborted frames.
- `can_uplink_tx_timeout`: requests canceled after 30ms waiting for completion.
- `can_uplink_mailbox_busy`: no free TX mailbox at enqueue attempt.
- `can_uplink_last_id`: latest attempted standard CAN ID.
- `can_uplink_last_esr`, `can_uplink_last_tsr`: CAN error and TX status registers.
- `can_uplink_rx_count` is reserved for future protocol integration; no increase is currently implemented.

## Test plan

1. Checkout `feature/can-adc-uplink-20261019`; open `Project.uvprojx`; Rebuild all and flash.
2. Verify program actually samples ADC values and `can_uplink_init_ok==1`.
3. Connect a compatible powered USB-CAN adapter/second active controller, same 500 kbit/s, to the external CANH/CANL interface. Check differential wiring, 120 ohm endpoint termination and isolated-side power.
4. Capture IDs 0x110–0x112. Decode four LE uint16 values from each 8-byte payload. Verify against Keil Watch filtered_adc variables and measured sensor inputs.
5. Observe `can_uplink_tx_ok` increase and `can_uplink_tx_fail` stay at 0 in stable conditions. Distinguish CAN ACK from controller *application-layer acceptance*; consider an explicit application ACK if required.
6. Confirm actual controller expects these IDs, scaling, send period and startup behavior before integrating into operational control.

## Important

This is a proposed sensor-to-controller uplink mapping, not a validated controller protocol. No control command arbitration, time synchronization, CRC above CAN, sequence counters, heartbeat, channel fault bitmap or acknowledgment at the application layer is defined. Agree on these items if required by the target controller.
