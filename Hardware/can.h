#ifndef __CAN_H
#define __CAN_H

#include "stm32f10x.h"

// CAN波特率预分频器定义
#define CAN_BAUDRATE_500K 4   // 36MHz/((8+9+1)*4) = 500Kbps

// CAN工作模式定义
#define CAN_MODE_NORMAL    0
#define CAN_MODE_LOOPBACK  1

// CAN过滤器模式定义
#define CAN_FILTER_MODE_IDMASK     0
#define CAN_FILTER_MODE_IDLIST     1

// CAN过滤器尺度定义
#define CAN_FILTER_SCALE_16BIT     0
#define CAN_FILTER_SCALE_32BIT     1

// 外部变量声明
extern u32 ControlID;
extern u32 ServoCmdID;

// 函数声明
u8 CAN_Mode_Init(u8 tsjw, u8 tbs2, u8 tbs1, u16 brp, u8 mode);
u8 Can_Send_Msg(u8* msg, u8 len);
u32 Can_Receive_Msg(u8 *buf);
void CAN_Init_Default(void);

// 中断相关函数
void USB_LP_CAN1_RX0_IRQHandler(void);

// 添加以下声明
void CAN_SetRxCallback(void (*pCallback)(CanRxMsg*));
uint8_t CAN_SendMessage(CanTxMsg* TxMessage);
uint8_t CAN_SendMessage_NoTimeout(CanTxMsg* TxMessage);
////uint8_t CAN_SendMessage_Timeout(CanTxMsg* TxMessage, uint32_t timeout);
void CAN_Config(uint32_t baudrate);
void CAN_EnableInterrupt(void);

uint8_t CAN_SendMessage_NonBlocking(CanTxMsg* TxMessage);
uint8_t CAN_CheckTransmitStatus(uint8_t mailbox);

// 在 can.h 中添加以下声明
#define CAN_RX0_INT_ENABLE 1  // 启用RX0中断
// 添加函数声明
extern void CAN_Rx_Callback(CanRxMsg* rx_msg);
extern void CAN_Receive_Polling(void);



#endif /* __CAN_H */
