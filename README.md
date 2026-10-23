# STM32F103C8T6 - 10-channel 4-20 mA acquisition and CAN upload

This branch is based on `main_ADC_CAN_from_v0_1023` and integrates the ADS1118 acquisition/CAN upload logic with runtime Watch diagnostics while retaining the STM32F103C8 project, startup files, and Standard Peripheral Library.

## Function
- 10 sensor inputs; each 4-20 mA signal is converted by a 100 ohm shunt to 0.4-2.0 V.
- Three ADS1118 devices, shared SPI1: PA5=SCK, PA6=MISO, PA7=MOSI.
- All three chip selects are active LOW: PB0=N_O1, PB1=N_O2, PB10=N_O3. HIGH means deselected.
- CAN1 default pins: PA11=RX, PA12=TX. Bit rate is configured for 500 kbit/s when APB1=36 MHz.
- Scan period: 100 ms.
- CAN standard data frames: 0x310, 0x311, 0x312 carry ten uint16 current values in microamps, little-endian; 0x313 carries valid mask, fault mask, sequence, transmit error count, protocol version and reserved byte.

## Keil
Open the root `Project.uvprojx` from this branch. It remains configured for STM32F103C8 and uses the existing `Hardware/adc420_*.c` source files. Rebuild the target before downloading.

## Debug
See `Debug/Watch_Variables.txt` for the volatile Watch variables. CAN transmit success requires a correctly wired CAN transceiver and at least one active ACKing node on the bus.
