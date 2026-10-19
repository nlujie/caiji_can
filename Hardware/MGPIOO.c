#include "stm32f10x.h"                  // Device header


void MGPIOO_Init(void)
{
	RCC_APB2PeriphClockCmd (RCC_APB2Periph_GPIOA, ENABLE );
	RCC_APB2PeriphClockCmd (RCC_APB2Periph_GPIOB, ENABLE );
	
	GPIO_InitTypeDef GPIO_InitStructure;
	
//输出高电平
	
	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_5 | GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init (GPIOB ,&GPIO_InitStructure);
	
	//GPIO_SetBits (GPIOB ,GPIO_Pin_0 | GPIO_Pin_1| GPIO_Pin_10 | GPIO_Pin_11);
	
	GPIO_SetBits (GPIOB ,GPIO_Pin_0 | GPIO_Pin_1| GPIO_Pin_5 | GPIO_Pin_6);
	//输入低电平
	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init (GPIOB ,&GPIO_InitStructure);
	
	

	
	
}
