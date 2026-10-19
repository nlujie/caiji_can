#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "filter.h"
//#include "MPU6050.h"
#include "func.h"
#include "ads1118_spi.h"
#include "can.h"
#include "MGPIOO.h"
//#include "serial1.h"
#include "dr154_dtu.h"
#include "modbus_rtu_slave.h"
#include "serial.h"
#include "serial1.h"
#include "serial2.h"
#include "stm32f10x_it.h"
#include <math.h>  // 用于数学函数
#include "func.h"
#include "modbus_rtu_master.h"

// 全局变量：系统时间戳
extern volatile uint32_t systemTick2;


extern void Serial_SendFloat(float value, int decimal_places);

// 全局变量声明
extern CAN_Message can_rx_msg;
extern uint32_t last_can_activity_time;


// CAN测试变量
uint32_t last_can_test_time = 0;
uint8_t can_test_data = 0;

//// ADC测量变量
//float original_voltage10 = 0.0f;
//float original_voltage11 = 0.0f;
//float original_voltage12 = 0.0f;
//float original_voltage13 = 0.0f;


//// ADC测量变量
//float original_voltage14 = 0.0f;
//float original_voltage15 = 0.0f;
//float original_voltage16 = 0.0f;
//float original_voltage17 = 0.0f;


//// 温度测量变量
//float temperature = 0.0f;  // 计算得到的温度值

//// NTC参数
//#define NTC_R25 10000.0f   // 25°C时的电阻值 (10KΩ)
//#define NTC_BETA 3435.0f   // Beta值
//#define NTC_T25 298.15f    // 25°C对应的开尔文温度 (25 + 273.15)

//// 参考电阻值（分压电路中的上拉电阻）
//#define REF_RESISTOR 10000.0f  // 10KΩ


// 正确的 SysTick 初始化
void SysTick_Init(void)
{
 
    
    
    // 配置 SysTick 为 1ms 中断一次
    if (SysTick_Config(SystemCoreClock / 1000)) {
        // 错误处理
        while (1);
    }
    
    // 设置 SysTick 中断优先级（可选）
    NVIC_SetPriority(SysTick_IRQn, 0);
    
}



// 获取当前时间戳（毫秒）
uint32_t GetTick(void)
{
    return systemTick2;
}


/*
// 获取当前时间戳（单位：毫秒）
uint32_t GetTick(void) {
    return systemTick;
}
*/

//// 根据电压计算NTC电阻值
//float CalculateNTCResistance(float voltage) {
//    // 假设使用分压电路：VCC -> REF_RESISTOR -> NTC -> GND
//    // 测量的是NTC两端的电压
//    
//    // 计算NTC电阻值: R_ntc = (V_ntc * R_ref) / (VCC - V_ntc)
//    // 这里假设VCC = 3.3V，但实际应根据电路调整
//    const float VCC = 4.0f;
//    
//    if (voltage >= VCC) {
//        return 0.0f; // 避免除以零
//    }
//    
//    return (voltage * REF_RESISTOR) / (VCC - voltage);
//}

//// 根据NTC电阻计算温度（使用Steinhart-Hart方程）
//float CalculateNTCTemperature(float resistance) {
//    if (resistance <= 0.0f) {
//        return -273.15f; // 绝对零度，表示错误
//    }
//    
//    // Steinhart-Hart方程: 1/T = 1/T0 + (1/B) * ln(R/R0)
//    // 其中:
//    // T - 开尔文温度
//    // T0 - 参考温度 (25°C = 298.15K)
//    // R - 当前电阻
//    // R0 - 参考电阻 (10KΩ)
//    // B - Beta值 (3435)
//    
//    float steinhart;
//    steinhart = resistance / NTC_R25;             // (R/R0)
//    steinhart = log(steinhart);                   // ln(R/R0)
//    steinhart /= NTC_BETA;                        // 1/B * ln(R/R0)
//    steinhart += 1.0 / NTC_T25;                   // 1/T0 + 1/B * ln(R/R0)
//    steinhart = 1.0 / steinhart;                  // 计算开尔文温度
//    
//    return steinhart - 273.15;                    // 转换为摄氏度
//}




//// 简单的ADS1118读取函数（在主循环中使用）
//void Read_ADS1118_Direct(void) {
//    uint16_t val0,val1,val2,val3;
//    float voltage,voltage1,voltage2,voltage3;
//    
//    uint16_t val4,val5,val6,val7;
//    float voltage4,voltage5,voltage6,voltage7;
//    
//    /*
//    // 读取SPI1的AIN0通道
//    val0 = ADS1118_ReadVotage_SPI1(ADS1118_CONFIG_AIN0);
//    voltage = val0 * 0.125f; // 转换为mV
//    original_voltage10 = ConvertToOriginalVoltage(voltage / 1000.0f);
//    
//    
//    Delay_ms(5);
//     // 读取SPI1的AIN1通道
//    val1 = ADS1118_ReadVotage_SPI1(ADS1118_CONFIG_AIN1);
//    voltage1 = val1 * 0.125f; // 转换为mV
//    original_voltage11 = ConvertToOriginalVoltage(voltage / 1000.0f);
//    
//    Delay_ms(5);
//     // 读取SPI1的AIN2通道
//    val2 = ADS1118_ReadVotage_SPI1(ADS1118_CONFIG_AIN2);
//    voltage2 = val2 * 0.125f; // 转换为mV
//    original_voltage12 = ConvertToOriginalVoltage(voltage / 1000.0f);
//    
//    Delay_ms(5);
//     // 读取SPI1的AIN3通道
//    val3 = ADS1118_ReadVotage_SPI1(ADS1118_CONFIG_AIN3);
//    voltage3 = val3 * 0.125f; // 转换为mV
//    original_voltage13 = ConvertToOriginalVoltage(voltage / 1000.0f);
//    
//    */
//    
//    /*
//    // 读取SPI1的AIN0通道
//    val4 = ADS1118_ReadVotage_SPI2(ADS1118_CONFIG_AIN0);
//    voltage4 = val4 * 0.125f; // 转换为mV
//    original_voltage14 = ConvertToOriginalVoltage(voltage / 1000.0f);
//    
//    
//    Delay_ms(5);
//   
//   // 读取SPI1的AIN1通道
//    val5 = ADS1118_ReadVotage_SPI2(ADS1118_CONFIG_AIN1);
//    voltage5 = val5 * 0.125f; // 转换为mV
//    original_voltage15 = ConvertToOriginalVoltage(voltage / 1000.0f);
//    
//    Delay_ms(5);
//    */
//     // 读取SPI1的AIN2通道
//    val6 = ADS1118_ReadVotage_SPI2(ADS1118_CONFIG_AIN2);
//    voltage6 = val6 * 0.125f; // 转换为mV
//    original_voltage16 = voltage6 / 1000.0f; // 转换为V
//   //// original_voltage16 = ConvertToOriginalVoltage(voltage / 1000.0f);
//   
//    // 计算NTC电阻值
//    float ntc_resistance = CalculateNTCResistance(original_voltage16);
//    
//    // 计算温度值
//    temperature = CalculateNTCTemperature(ntc_resistance);
//    
//    Delay_ms(5);
//   /* 
//   // 读取SPI1的AIN3通道
//    val7 = ADS1118_ReadVotage_SPI2(ADS1118_CONFIG_AIN3);
//    voltage7 = val7 * 0.125f; // 转换为mV
//    original_voltage17 = ConvertToOriginalVoltage(voltage / 1000.0f);
//    
//    
//    
//    // 打印结果
//    Serial_SendString("ADC Value0: ");
//    Serial_SendNumber(val0);
//    Serial_SendString(" (");
//    Serial_SendFloat(voltage, 1); // 显示mV值，保留1位小数
//    Serial_SendString("mV, ");
//  ////  Serial_SendFloat(original_voltage10, 3); // 显示V值，保留3位小数
//  // // Serial_SendString("V)\r\n");
//    Serial_SendString("\r\n");
//    
//     // 打印结果
//    Serial_SendString("ADC Value1: ");
//    Serial_SendNumber(val1);
//    Serial_SendString(" (");
//    Serial_SendFloat(voltage1, 1); // 显示mV值，保留1位小数
//    Serial_SendString("mV, ");
//    ////Serial_SendFloat(original_voltage11, 3); // 显示V值，保留3位小数
//   //// Serial_SendString("V)\r\n");
//    Serial_SendString("\r\n");
//   
//    
//     // 打印结果
//    Serial_SendString("ADC Value2: ");
//    Serial_SendNumber(val2);
//    Serial_SendString(" (");
//    Serial_SendFloat(voltage2, 1); // 显示mV值，保留1位小数
//    Serial_SendString("mV, ");
//    ////Serial_SendFloat(original_voltage12, 3); // 显示V值，保留3位小数
//   //// Serial_SendString("V)\r\n");
//    Serial_SendString("\r\n");
//    
//     // 打印结果
//    Serial_SendString("ADC Value3: ");
//    Serial_SendNumber(val3);
//    Serial_SendString(" (");
//    Serial_SendFloat(voltage3, 1); // 显示mV值，保留1位小数
//    Serial_SendString("mV, ");
//    //// Serial_SendFloat(original_voltage13, 3); // 显示V值，保留3位小数
//   //// Serial_SendString("V)\r\n");
//    Serial_SendString("\r\n");
//    
//    */
//    /*
//    // 打印结果
//    Serial_SendString("ADC Value4: ");
//    Serial_SendNumber(val4);
//    Serial_SendString(" (");
//    Serial_SendFloat(voltage4, 1); // 显示mV值，保留1位小数
//    Serial_SendString("mV, ");
//  ////  Serial_SendFloat(original_voltage10, 3); // 显示V值，保留3位小数
//  // // Serial_SendString("V)\r\n");
//    Serial_SendString("\r\n");
//    
//     // 打印结果
//    Serial_SendString("ADC Value5: ");
//    Serial_SendNumber(val5);
//    Serial_SendString(" (");
//    Serial_SendFloat(voltage5, 1); // 显示mV值，保留1位小数
//    Serial_SendString("mV, ");
//    ////Serial_SendFloat(original_voltage11, 3); // 显示V值，保留3位小数
//   //// Serial_SendString("V)\r\n");
//    Serial_SendString("\r\n");
//   */
//    
//     // 打印结果
//    Serial_SendString("ADC Value6: ");
//   // Serial_SendNumber(val6);
//    Serial_SendString(" (");
//    Serial_SendFloat(voltage6, 1); // 显示mV值，保留1位小数
//    Serial_SendString("mV, ");
//    Serial_SendFloat(original_voltage16, 3); // 显示V值，保留3位小数
//    Serial_SendString("V) ");
//    Serial_SendString("NTC Resistance: ");
//    Serial_SendFloat(ntc_resistance, 1); // 显示电阻值，保留1位小数
//    Serial_SendString("Ω ");
//    Serial_SendString("Temperature: ");
//    Serial_SendFloat(temperature, 1); // 显示温度值，保留1位小数
//    Serial_SendString("°C\r\n");
//    
//    
//   // Serial_SendString("V)\r\n");
//    Serial_SendString("\r\n");
//    /*
//     // 打印结果
//    Serial_SendString("ADC Value7: ");
//    Serial_SendNumber(val7);
//    Serial_SendString(" (");
//    Serial_SendFloat(voltage7, 1); // 显示mV值，保留1位小数
//    Serial_SendString("mV, ");
//    //// Serial_SendFloat(original_voltage13, 3); // 显示V值，保留3位小数
//   //// Serial_SendString("V)\r\n");
//    Serial_SendString("\r\n");
//    */
//    
//}


extern uint8_t timer3_flag;
// 时钟诊断函数

int main(void)
{  
   SystemInit();
    SysTick_Init();
    
    // 检查看门狗复位
 ////   Check_IWDG_Reset();
    
    // 初始化看门狗（2秒超时）
   //// IWDG_Init(2000);
    
    


// 初始化GPIO
    MGPIOO_Init();
    ADS1118_Init();
    
    TIM3_Int_Init(10 - 1,7200 - 1);	

//   
    Serial_Init();
//    Serial_Init_1();
    Serial_Init_2();
    // 初始化USART1 (波特率115200)
    //Serial_Init_1();
   
  
// Serial_SendString("1System Boot OK\r\n");
 //   Serial_SendString_1("2System Boot OK\r\n");
////    Serial_SendString_2("3System Boot OK\r\n");
    
    
    // 温湿度传感器初始化
////    HTM440_Init();
////    
////    // 设置温湿度读取间隔（2秒）
////    HTM440_SetReadInterval(2000);
    
////     // 初始化复合滤波结构体
    Initialize_Composite_Filter_Structs();

//// 
////    

////    
//// 
//// 
        CAN_Module_Init();

uint16_t val;
        float value;

        uint32_t last_send_time = 0;
        
          uint32_t last_100ms = 0;
    uint32_t last_200ms = 0;
        
        
	while(1)
	{
        
         // 必须添加Modbus轮询
     ////   ModbusRTU_Poll();
        
        
//        Delay_us(100);
//		val = ADS1118_ReadVotage_SPI1_Chip1(0x44eb);//使用AIN0-GND
//		Delay_ms(3000);
//		Serial_SendString_2("mA(");
//    Serial_SendNumber_2(val);
//    Serial_SendString_2(",");
//    
//		value = val*0.0625;
//        Serial_SendFloat(value, 3);  // 添加电压显示，3位小数
//    Serial_SendString_2("V) ");
		 
        
        
////        // 检查定时器标志
////    if(timer3_flag) {
////        timer3_flag = 0;
////        Control_Task();  // 在主循环中执行控制任务
////    }
////        
////        // 每秒发送一次CAN消息
////        if(GetTick() - last_send_time >= 10) {
////          last_send_time = GetTick();
////        ////   CAN_Send_Example(); // 发送CAN消息
////            
////            // CAN发送后喂狗
////           //// IWDG_Feed();
////       }
        
        
        uint32_t current_time = task_tim1.time_1ms;
        
        // 100ms任务
        if(current_time - last_100ms >= 100) {
            last_100ms = current_time;
            task_100ms();
        }
        
        // 200ms任务
        if(current_time - last_200ms >= 200) {
            last_200ms = current_time;
            task_200ms();
        }
        
        
        // 直接读取ADS1118
        ////Read_ADS1118_Direct();
        
         // 短暂延时
        Delay_us(1); // 每hao秒读取一次
    
	}
}
