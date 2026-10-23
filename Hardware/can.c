#include "can.h"
#include "func.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_can.h"
#include <stddef.h>


// 添加这些 extern 声明
extern volatile Task_Timer task_tim1;
extern CAN_Message can_rx_msg;
extern uint32_t last_can_activity_time; // 需要将 func.c 中的 static 移除
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK战舰STM32开发板
//CAN驱动 代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2014/5/7
//版本：V1.1 
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2014-2024
//All rights reserved	
//********************************************************************************
//V1.1修改说明 20150528
//修正了CAN初始化函数的相关注释，更正了波特率计算公式
////////////////////////////////////////////////////////////////////////////////// 	 
 
// 声明外部变量
//extern Task_Timer task_tim1;
//extern CAN_Message can_rx_msg;
 
//CAN初始化
//tsjw:重新同步跳跃时间单元.范围:CAN_SJW_1tq~ CAN_SJW_4tq
//tbs2:时间段2的时间单元.   范围:CAN_BS2_1tq~CAN_BS2_8tq;
//tbs1:时间段1的时间单元.   范围:CAN_BS1_1tq ~CAN_BS1_16tq
//brp :波特率分频器.范围:1~1024;  tq=(brp)*tpclk1
//波特率=Fpclk1/((tbs1+1+tbs2+1+1)*brp);
//mode:CAN_Mode_Normal,普通模式;CAN_Mode_LoopBack,回环模式;
//Fpclk1的时钟在初始化的时候设置为36M,如果设置CAN_Mode_Init(CAN_SJW_1tq,CAN_BS2_8tq,CAN_BS1_9tq,4,CAN_Mode_LoopBack);
//则波特率为:36M/((8+9+1)*4)=500Kbps
//返回值:0,初始化OK;
//    其他,初始化失败; 

u32 ControlID = 0;	
u32 ServoCmdID = 0;
//u32 testid2 = 0;
u8 CAN_Mode_Init(u8 tsjw,u8 tbs2,u8 tbs1,u16 brp,u8 mode)
{ 
	GPIO_InitTypeDef 		GPIO_InitStructure; 
	CAN_InitTypeDef        	CAN_InitStructure;
	CAN_FilterInitTypeDef  	CAN_FilterInitStructure;
#if CAN_RX0_INT_ENABLE 
	NVIC_InitTypeDef  		NVIC_InitStructure;
#endif

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);//使能GPIOA/AFIO时钟
    /* 本采集板 CAN1 实际连PA11(RX)、PA12(TX)，明确禁止CAN引脚重映射。 */
    GPIO_PinRemapConfig(GPIO_Remap1_CAN1, DISABLE);
    GPIO_PinRemapConfig(GPIO_Remap2_CAN1, DISABLE);	                   											 

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);//使能CAN1时钟	

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//复用推挽
	GPIO_Init(GPIOA, &GPIO_InitStructure);			//初始化IO

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;	//上拉输入
	GPIO_Init(GPIOA, &GPIO_InitStructure);			//初始化IO

	//CAN单元设置
	CAN_InitStructure.CAN_TTCM=DISABLE;			//非时间触发通信模式  
	CAN_InitStructure.CAN_ABOM=ENABLE;			//软件自动离线管理	 
	CAN_InitStructure.CAN_AWUM=DISABLE;			//睡眠模式通过软件唤醒(清除CAN->MCR的SLEEP位)
	CAN_InitStructure.CAN_NART=ENABLE;			//禁止报文自动传送 
	CAN_InitStructure.CAN_RFLM=DISABLE;		 	//报文不锁定,新的覆盖旧的  
	CAN_InitStructure.CAN_TXFP=DISABLE;			//优先级由报文标识符决定 
	CAN_InitStructure.CAN_Mode= mode;	        //模式设置： mode:0,普通模式;1,回环模式; 
	//设置波特率
	CAN_InitStructure.CAN_SJW=tsjw;				//重新同步跳跃宽度(Tsjw)为tsjw+1个时间单位  CAN_SJW_1tq	 CAN_SJW_2tq CAN_SJW_3tq CAN_SJW_4tq
	CAN_InitStructure.CAN_BS1=tbs1; 			//Tbs1=tbs1+1个时间单位CAN_BS1_1tq ~CAN_BS1_16tq
	CAN_InitStructure.CAN_BS2=tbs2;				//Tbs2=tbs2+1个时间单位CAN_BS2_1tq ~	CAN_BS2_8tq
	CAN_InitStructure.CAN_Prescaler=brp;        //分频系数(Fdiv)为brp+1	
	if(CAN_Init(CAN1, &CAN_InitStructure) == CAN_InitStatus_Failed)
        return 1U;  /* 初始化失败不再假装CAN已正常启动 */

	
    // 配置过滤器0 - 接收所有标准ID消息
    CAN_FilterInitStructure.CAN_FilterNumber = 0;
    CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;
    CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;
    CAN_FilterInitStructure.CAN_FilterIdHigh = 0x0000;
    CAN_FilterInitStructure.CAN_FilterIdLow = 0x0000;
    CAN_FilterInitStructure.CAN_FilterMaskIdHigh = 0x0000;
    CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0x0000;
    CAN_FilterInitStructure.CAN_FilterFIFOAssignment = CAN_Filter_FIFO0;
    CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;
    CAN_FilterInit(&CAN_FilterInitStructure);
    
   /* 
	CAN_FilterInitStructure.CAN_FilterNumber = 0;	  						//过滤器0
  	CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdList; 		//工作在列表模式
  	CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;		//筛选器位宽为单个32位
    */
	/*
	switch(Slave.Index)
	{
		case Chip1:
		{
			U13 II电机左
			ControlID = (((u32)0x000C<<21)|CAN_ID_STD|CAN_RTR_DATA);	
			ServoCmdID = (((u32)0x000C<<21)|CAN_ID_STD|CAN_RTR_DATA);
			break;	
		}
		case Chip2:
		{
			T-motor电机左
			ControlID = (((u32)0x000C<<21)|CAN_ID_STD|CAN_RTR_DATA);	
			ServoCmdID = (((u32)0x020C<<21)|CAN_ID_STD|CAN_RTR_DATA);
			break;	
		}	
		case Chip3:
		{
			T-motor电机右
			ControlID = (((u32)0x010C<<21)|CAN_ID_STD|CAN_RTR_DATA);
			ServoCmdID = (((u32)0x020C<<21)|CAN_ID_STD|CAN_RTR_DATA);
			break;	
		}	
		case Chip4:
		{
			U13 II电机右
			ControlID = (((u32)0x010C<<21)|CAN_ID_STD|CAN_RTR_DATA);
			ServoCmdID = (((u32)0x010C<<21)|CAN_ID_STD|CAN_RTR_DATA);
			break;	
		}
	}
	*/
	/* 参考手册P617筛选器组尺度配置 - 寄存器构成 
	使能筛选器，按照标志的内容进行比对筛选，标准ID，是的话，会存入FIFO0。*/
    /*
  	CAN_FilterInitStructure.CAN_FilterIdHigh = (ControlID&0xFFFF0000)>>16;	//要筛选的ID高位 
  	CAN_FilterInitStructure.CAN_FilterIdLow = ControlID&0xFFFF; 				//要筛选的ID低位 
	
	CAN_FilterInitStructure.CAN_FilterMaskIdHigh = (ServoCmdID&0xFFFF0000)>>16;//32位MASK
	CAN_FilterInitStructure.CAN_FilterMaskIdLow = ServoCmdID&0xFFFF;
	
	CAN_FilterInitStructure.CAN_FilterFIFOAssignment = CAN_Filter_FIFO0;	//过滤器0关联到FIFO0
  	CAN_FilterInitStructure.CAN_FilterActivation = ENABLE; 					//激活过滤器0
  	CAN_FilterInit(&CAN_FilterInitStructure);								//滤波器初始化
        */
/*
	CAN_FilterInitStructure.CAN_FilterNumber = 1;	  						//过滤器1
  	CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdList; 		//工作在
  	CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;		//筛选器位宽为单个32位

	CAN_FilterInitStructure.CAN_FilterFIFOAssignment=CAN_Filter_FIFO0;		//过滤器0关联到FIFO0
	CAN_FilterInitStructure.CAN_FilterActivation=ENABLE;					//激活过滤器0

	CAN_FilterInit(&CAN_FilterInitStructure);								//滤波器初始化
*/	
#if CAN_RX0_INT_ENABLE 
	CAN_ITConfig(CAN1,CAN_IT_FMP0,ENABLE);				//FIFO0消息挂号中断允许.		    

	NVIC_InitStructure.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;     // 主优先级为1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;            // 次优先级为0
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
#endif
	return 0;
}  






 /*
#if CAN_RX0_INT_ENABLE	//使能RX0中断
//中断服务函数			    
void USB_LP_CAN1_RX0_IRQHandler(void)
{
  	CanRxMsg RxMessage;
	int i=0;
    CAN_Receive(CAN1, 0, &RxMessage);
	
	// 将接收到的消息存入全局变量
    can_rx_msg.id = RxMessage.StdId;
    can_rx_msg.format = RxMessage.IDE;
    can_rx_msg.type = RxMessage.RTR;
    can_rx_msg.len = RxMessage.DLC;
    for(i=0;i<RxMessage.DLC;i++)
        can_rx_msg.data[i] = RxMessage.Data[i];
    
    // 更新最后活动时间
    last_can_activity_time = task_tim1.time_1ms;
	
	// 调试输出
	for(i=0;i<8;i++)
	printf("rxbuf[%d]:%d\r\n",i,RxMessage.Data[i]);
}
#endif
*/
//can发送一组数据(固定格式:ID为0X12,标准帧,数据帧)	
//len:数据长度(最大为8)				     
//msg:数据指针,最大为8个字节.
//返回值:0,成功;
//		 其他,失败;
u8 Can_Send_Msg(u8* msg,u8 len)
{	
	u8 mbox;
	u16 i=0;
	CanTxMsg TxMessage;
	TxMessage.StdId=0x12;			// 标准标识符 
	TxMessage.ExtId=0x12;			// 设置扩展标示符 
	TxMessage.IDE=CAN_Id_Standard; 	// 标准帧
	TxMessage.RTR=CAN_RTR_Data;		// 数据帧
	TxMessage.DLC=len;				// 要发送的数据长度
	for(i=0;i<len;i++)
	TxMessage.Data[i]=msg[i];			          
	mbox= CAN_Transmit(CAN1, &TxMessage);   
	i=0; 
	while((CAN_TransmitStatus(CAN1, mbox)==CAN_TxStatus_Failed)&&(i<0XFFF))i++;	//等待发送结束
	if(i>=0XFFF)return 1;
	return 0;	 
}

//can口接收数据查询
//buf:数据缓存区;	 
//返回值:0,无数据被收到;
//		 其他,接收的数据长度;
u32 Can_Receive_Msg(u8 *buf)
{		   		   
 	u32 i;
	CanRxMsg RxMessage;
    if( CAN_MessagePending(CAN1,CAN_FIFO0)==0)return 0;		//没有接收到数据,直接退出 
    CAN_Receive(CAN1, CAN_FIFO0, &RxMessage);//读取数据	
    for(i=0;i<8;i++)
    buf[i]=RxMessage.Data[i];  
	return RxMessage.StdId;	
}


static void (*RxCallback)(CanRxMsg*) = NULL;

// CAN初始化
void CAN_Config(uint32_t baudrate) {
    GPIO_InitTypeDef GPIO_InitStructure;
    CAN_InitTypeDef CAN_InitStructure;
    
    // 使能时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);
    
    // 配置CAN引脚: PA11-RX, PA12-TX
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // CAN参数配置
    CAN_InitStructure.CAN_TTCM = DISABLE;
    CAN_InitStructure.CAN_ABOM = ENABLE;  // 启用自动离线管理
    CAN_InitStructure.CAN_AWUM = ENABLE;  // 启用自动唤醒模式
    CAN_InitStructure.CAN_NART = DISABLE;
    CAN_InitStructure.CAN_RFLM = DISABLE;
    CAN_InitStructure.CAN_TXFP = DISABLE;
    CAN_InitStructure.CAN_Mode = CAN_Mode_Normal;
    CAN_InitStructure.CAN_SJW = CAN_SJW_1tq;
    CAN_InitStructure.CAN_BS1 = CAN_BS1_8tq;
    CAN_InitStructure.CAN_BS2 = CAN_BS2_7tq;
    
    // 正确计算波特率分频值
    // APB1时钟通常是36MHz (当系统时钟为72MHz时)
    // 500kbps = 36MHz / (分频 * (1 + BS1 + BS2))
    // 分频 = 36MHz / (500kHz * (1 + 8 + 7)) = 36MHz / (500kHz * 16) = 36MHz / 8MHz = 4.5
    // 取整为4或5，这里选择4
    CAN_InitStructure.CAN_Prescaler = 4;
    
    // 如果提供了波特率参数，则使用它
    if(baudrate > 0) {
        // 假设APB1时钟为36MHz
        uint32_t apb1_clock = 36000000;
        CAN_InitStructure.CAN_Prescaler = apb1_clock / (baudrate * (1 + 8 + 7));
        if(CAN_InitStructure.CAN_Prescaler < 1) CAN_InitStructure.CAN_Prescaler = 1;
    }
    
    // 初始化CAN
    if(CAN_Init(CAN1, &CAN_InitStructure) != CANINITFAILED) {
        // 初始化成功
    } else {
        // 初始化失败，可以在这里处理错误
        while(1);
    }
}

extern uint32_t GetTick(void);





// 不带超时的发送消息函数 - 立即返回发送状态
uint8_t CAN_SendMessage_NoTimeout(CanTxMsg* TxMessage) {
    uint8_t transmit_mailbox = 0;
    
    TxMessage->IDE = CAN_ID_STD; // 标准ID
    TxMessage->RTR = CAN_RTR_DATA; // 数据帧
    
    transmit_mailbox = CAN_Transmit(CAN1, TxMessage);
    
    if(transmit_mailbox == CAN_TxStatus_NoMailBox) {
        // 没有可用邮箱，立即返回失败
        return CAN_TxStatus_Failed;
    }
    
    // 立即返回发送状态（不等待完成）
    return CAN_TransmitStatus(CAN1, transmit_mailbox);
}

// 原始发送消息函数（保持兼容性）- 修改为不带超时
uint8_t CAN_SendMessage(CanTxMsg* TxMessage) {
    return CAN_SendMessage_NoTimeout(TxMessage);
}



/*
// 带超时的发送消息函数
uint8_t CAN_SendMessage_Timeout(CanTxMsg* TxMessage, uint32_t timeout) {
    uint8_t transmit_mailbox = 0;
    uint32_t start_time = GetTick();
    
    TxMessage->IDE = CAN_ID_STD; // 标准ID
    TxMessage->RTR = CAN_RTR_DATA; // 数据帧
    
    transmit_mailbox = CAN_Transmit(CAN1, TxMessage);
    
    if(transmit_mailbox == CAN_TxStatus_NoMailBox) {
        // 没有可用邮箱，等待超时
        while((GetTick() - start_time) < timeout) {
            if(CAN_TransmitStatus(CAN1, transmit_mailbox) != CAN_TxStatus_Failed) {
                return CAN_TxStatus_Ok;
            }
        }
        return CAN_TxStatus_Failed;
    }
    
    // 等待发送完成或超时
    while((GetTick() - start_time) < timeout) {
        if(CAN_TransmitStatus(CAN1, transmit_mailbox) == CAN_TxStatus_Ok) {
            return CAN_TxStatus_Ok;
        }
    }
    
    return CAN_TxStatus_Failed;
}

// 原始发送消息函数（保持兼容性）
uint8_t CAN_SendMessage(CanTxMsg* TxMessage) {
    return CAN_SendMessage_Timeout(TxMessage, 100); // 100ms超时
}
*/








// 设置过滤器
void CAN_SetFilter(CAN_FilterInitTypeDef* CAN_FilterInitStruct) {
    CAN_FilterInit(CAN_FilterInitStruct);
}

// 设置接收回调
void CAN_SetRxCallback(void (*pCallback)(CanRxMsg*)) {
    RxCallback = pCallback;
}

// 使能中断
void CAN_EnableInterrupt(void) {
    NVIC_InitTypeDef NVIC_InitStructure;
    
    NVIC_InitStructure.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    CAN_ITConfig(CAN1, CAN_IT_FMP0, ENABLE);
}

// CAN中断处理 - 修正函数名
void USB_LP_CAN1_RX0_IRQHandler(void) {
    CanRxMsg RxMessage;
    
    if(CAN_GetITStatus(CAN1, CAN_IT_FMP0) != RESET) {
        CAN_ClearITPendingBit(CAN1, CAN_IT_FMP0);
        
        CAN_Receive(CAN1, CAN_FIFO0, &RxMessage);
        
        if(RxCallback != NULL) {
            RxCallback(&RxMessage);
        }
    }
}
