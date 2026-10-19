# CAN1 internal loopback diagnostic

Base branch: `main_can_v0_1019_1504`
Test branch: `test/can1-internal-loopback-20261019`
Target: STM32F103C8T6, CAN1 PA11 (RX), PA12 (TX). Keil MDK ARMCC 5, STM32F10x StdPeriph Library.

## Usage

1. Checkout this test branch and open `Project.uvprojx` in Keil.
2. Build and program the STM32F103C8T6. **Do not use this image for production control**: `main()` deliberately stops after the CAN test; ADC and Modbus tasks are not run.
3. In Keil Debug / Watch 1 add: `can_selftest_status`, `can_selftest_pass`, `can_selftest_fail`, `can_selftest_last_seq`, `can_selftest_last_esr`, `can_selftest_last_tsr`, `can_selftest_pclk1`, `can_selftest_prescaler`, `can_selftest_tx_status`, `can_selftest_rx_id`, `can_selftest_rx_dlc`.
4. Optionally connect a **3.3 V compatible** UART adapter to PA9 TX and MCU DGND. Listen at **9600 8N1** (PA10 RX not required).
5. After reset, expect `can_selftest_status == 2`, `can_selftest_pass == 100`, `can_selftest_fail == 0`. A status other than 2 indicates failure. The status is also printed every 10 frames.

## Test design and interpretation

- The test uses bxCAN **internal** loopback (`CAN_Mode_LoopBack`, LBKM=1, SILM=0), FIFO0 polling, a receive-all filter and bounded TX/RX polling. No second node and no ACK from an external bus are required.
- The test dynamically reads APB1 (PCLK1) and calculates a prescaler for 500 kbit/s with 18 TQ/bit (BS1=15, BS2=2). If PCLK1 cannot be divided exactly, it reports status 3 rather than silently using the wrong bitrate.
- It transmits 100 standard data frames, cycling CAN IDs 0x321–0x330 and payload DLC 0–8, compares ID, frame type, DLC and every valid data byte on receive.
- Internal loopback PASS checks the MCU CAN controller, software configuration and local TX/RX path **inside the controller only**. It does **NOT** prove that the optically isolated external CAN transceiver, isolated 5 V supply, common-mode choke, protection diodes, differential CANH/CANL wiring, termination, or external ACK work.

## Status codes

| status | Meaning |
|---:|---|
| 0 | Not started |
| 1 | Running |
| 2 | PASS: all 100 frames verified |
| 3 | APB1 clock incompatible with exact 500 kbit/s configuration |
| 4 | CAN peripheral initialization failed |
| 5 | No TX mailbox available |
| 6 | CAN TX failure or timeout |
| 7 | No CAN RX frame before polling limit |
| 8 | Received CAN frame differs from transmitted frame |

## Hardware phase required to confirm the full circuit

Use a **second CAN node or USB-CAN analyzer** on the external connector with the board powered correctly. Set 500 kbit/s, verify termination (typically 120 ohms at each end of a bus), wiring polarity, common reference requirements of the selected transceiver and isolation barrier supply. With a second live node to ACK, use CAN **normal mode** to send and receive known frames. Check that the physical bus sees the frames and the controller's TEC/REC/error-state stay valid. This is a separate test and is not performed by this firmware.

## Existing code observations

- In the baseline project `CAN_Module_Init()` initializes CAN1 in normal mode and enables receive interrupts; `task_200ms()` sends ADC data. This diagnostic bypasses those tasks to make the result deterministic.
- Baseline `CAN_SendMessage_NoTimeout()` checks `CAN_TransmitStatus()` immediately, which can return PENDING; this diagnostic polls up to a bounded timeout instead.
- The original branch is preserved. Restore it to return to production behavior.
