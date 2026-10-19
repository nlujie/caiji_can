#include "stm32f10x.h"

/**
  * @brief  寰绾у欢鏃?
  * @param  xus 寤舵椂鏃堕暱锛岃寖鍥达細0~233015
  * @retval 鏃?
  */
void Delay_us(uint32_t xus)
{
	SysTick->LOAD = 72 * xus;				//璁剧疆瀹氭椂鍣ㄩ噸瑁呭€?
	SysTick->VAL = 0x00;					//娓呯┖褰撳墠璁℃暟鍊?
	SysTick->CTRL = 0x00000005;				//璁剧疆鏃堕挓婧愪负HCLK锛屽惎鍔ㄥ畾鏃跺櫒
	while(!(SysTick->CTRL & 0x00010000));	//绛夊緟璁℃暟鍒?
	SysTick->CTRL = 0x00000004;				//鍏抽棴瀹氭椂鍣?
}

/**
  * @brief  姣绾у欢鏃?
  * @param  xms 寤舵椂鏃堕暱锛岃寖鍥达細0~4294967295
  * @retval 鏃?
  */
void Delay_ms(uint32_t xms)
{
	while(xms--)
	{
		Delay_us(1000);
	}
}
 
/**
  * @brief  绉掔骇寤舵椂
  * @param  xs 寤舵椂鏃堕暱锛岃寖鍥达細0~4294967295
  * @retval 鏃?
  */
void Delay_s(uint32_t xs)
{
	while(xs--)
	{
		Delay_ms(1000);
	}
} 
