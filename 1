#ifndef __DR154_DTU_H
#define __DR154_DTU_H

#include "stm32f10x.h"
#include <string.h>
#include <stdio.h>

// 接收缓冲区大小
#define DR154_RX_BUF_SIZE 512
// 最大AT指令响应等待时间(ms)
#define DR154_RESP_TIMEOUT 5000

// DTU状态枚举
typedef enum {
    DR154_STATE_DISCONNECTED = 0,
    DR154_STATE_CONNECTING,
    DR154_STATE_CONNECTED
} DR154_State_t;

// DTU结构体
typedef struct {
    DR154_State_t state;
    uint8_t signal_quality; // 信号质量(0-31)
    uint8_t net_status;     // 网络注册状态
    uint32_t last_heartbeat;// 最后心跳时间
} DR154_Handle_t;

// 函数声明
void DR154_Init(void);
void DR154_Reset(void);
uint8_t DR154_Check_Connection(void);
void DR154_Send_Data(uint8_t *data, uint16_t len);
void DR154_Process(void);
void DR154_Send_Sensor_Data(void);

#endif /* __DR154_DTU_H */

