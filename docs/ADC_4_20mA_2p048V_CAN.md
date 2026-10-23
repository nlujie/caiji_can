# 10路4–20mA采集与CAN上报（ADS1118，PGA ±2.048V）

此说明对应分支 `main_ADC_4_20mA_CAN_1023_0929`，以项目内《采集板使用说明》通道映射为依据。

## 采样原理与配置

- 外部电流先流过PCB采样电阻，形成电压输入ADS1118；转换完成后使用SPI1读取16位有符号ADC码。
- ADS1118内部基准电路，**PGA选择±2.048V档位**，16位有符号输出的正向满量程约32768码，1 LSB=0.0000625V。
- 当前代码把采样电阻 **默认设为100Ω**（继承原工程中`SHUNT_RESISTOR_100OHM=100.0f`的假设）。**《使用说明》未明确列出该电阻的实装阻值，请实测/核对原理图**。如果PCB使用102.4Ω或其他阻值，改`Hardware/func.h`内的`ADC420_SHUNT_RESISTOR_OHM`。
- 正常4–20mA对应100Ω采样电阻两端**0.400–2.000V**，**不是0.400–2.048V**。2.048V是此ADC增益配置的正向量程上限。

### 转换公式

```text
voltage_v = (int16_t)adc_raw * 2.048 / 32768
current_ma = voltage_v * 1000 / ADC420_SHUNT_RESISTOR_OHM
can_word = round(current_ma * 100)    // uint16, 单位0.01mA
```

正常区间转换后的字值为400–2000。超出4.00–20.00mA量化区间、ADC为负码或无法表示时上报`0xFFFF`（无效）；请勿将`0xFFFF`除以100当成真实电流。

### 100Ω电阻时的验算

| 输入电流 | ADC端电压 | ADS1118原始码 | CAN字值 |
|---:|---:|---:|---:|
| 4.00mA | 0.400V | 6400 | 400 (0x0190) |
| 12.00mA | 1.200V | 19200 | 1200 (0x04B0) |
| 20.00mA | 2.000V | 32000 | 2000 (0x07D0) |

## SPI通道/片选映射

| ADC编号 | 插座 | 芯片片选 | 芯片通道 |
|---|---|---|---|
| ADC_2 | P10 | N_O2=PB1 | AIN2 |
| ADC_3 | P11 | N_O2=PB1 | AIN3 |
| ADC_4 | P1 | N_O1=PB0 | AIN0 |
| ADC_5 | P2 | N_O1=PB0 | AIN1 |
| ADC_6 | P4 | N_O1=PB0 | AIN3 |
| ADC_7 | P5 | N_O1=PB0 | AIN2 |
| ADC_8 | P9 | N_O3=PB10 | AIN2 |
| ADC_9 | P7 | N_O3=PB10 | AIN1 |
| ADC_10 | P8 | N_O3=PB10 | AIN3 |
| ADC_11 | P6 | N_O3=PB10 | AIN0 |

ADC_0/ADC_1为执行器反馈，不属于上述10路。

## CAN报文定义（保留当前协议）

每200ms目标发送一轮，CAN1标准帧，500kbps（依现有初始化），3帧/轮，DLC=8，各字为小端序uint16，单位0.01mA。

| CAN ID | 字0（Byte0-1） | 字1（Byte2-3） | 字2（Byte4-5） | 字3（Byte6-7） |
|---|---|---|---|---|
| 0x120 | ADC_2 | ADC_3 | ADC_4 | ADC_5 |
| 0x121 | ADC_6 | ADC_7 | ADC_8 | ADC_9 |
| 0x122 | ADC_10 | ADC_11 | 0xFFFF（保留） | 0xFFFF（保留） |

接收端用`(uint16_t)data[2*n] | ((uint16_t)data[2*n+1] << 8)`解出一个字值，再判断是否为`0xFFFF`；有效则除以100得到mA。

### Keil Watch

- `adc420_raw_samples[0..9]`：ADC原始值，对应ADC_2到ADC_11；
- `adc420_voltage_v[0..9]`：ADC端计算电压(V)；
- `adc420_current_ma[0..9]`：计算出的传感器电流(mA)，包括超量程观测值；
- `adc420_current_centi_ma[0..9]`：待发送CAN量化值，可能为`0xFFFF`。

请用已知4mA、12mA、20mA标准电流源及真实硬件测试，观察SPI数值、电压/电流换算与CAN接收是否一致。目前修改属于源码级校正，非板载实测或Keil编译通过的证明。
