# STM32F407VGT6 ADC + CAN acquisition

以 `main_ADC_CAN_from_v0_1023` 为基础重构根目录 Keil 工程。根目录 `Project.uvprojx` 现配置为 STM32F407VG，并且只编译 `F407_ADC_CAN/` 中的 F407 启动、时基、ADS1118 SPI、CAN1、采集应用与 Debug 监控源码。旧的 STM32F103 `User/main.c` 已从本分支移除，不再编译旧 F103 程序。

功能：10 路 4–20 mA 输入经 100 Ω 转换为 0.4–2.0 V；三片 ADS1118；SPI1 PA5/PA6/PA7；低有效片选 PB0/PB1/PB10；CAN1 PA11/PA12，500 kbit/s，报文 0x310–0x313。

优先打开仓库根目录 `Project.uvprojx`。需要在本机使用目标调试器和硬件完成 Flash 下载验证。