#include "modbus_rtu_master.h"
#include "Delay.h"
#include "string.h"
#include "func.h"
#include "serial2.h"

// 注意：这里不再定义 modbus_rx_complete，因为在 serial.c 中已经定义了
// 只在这里声明温湿度数据全局变量
HTM440_Data_t htm440_data = {0};

// 添加一个全局变量记录接收超时开始时间
static uint32_t modbus_receive_start_time = 0;
static uint8_t modbus_receiving = 0;



// 计算CRC16校验码
uint16_t Modbus_CRC16(uint8_t *data, uint16_t length)
{
    uint16_t crc = 0xFFFF;
    uint16_t i, j;
    
    for(i = 0; i < length; i++) {
        crc ^= (uint16_t)data[i];
        for(j = 0; j < 8; j++) {
            if(crc & 0x0001) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    
    return crc;
}

// 发送请求帧
uint8_t ModbusMaster_SendRequest(uint8_t slave_addr, uint8_t function, 
                                uint16_t start_addr, uint16_t reg_count)
{
    uint8_t frame[8];
    uint16_t crc;
    
    // 构建请求帧
    frame[0] = slave_addr;             // 从站地址
    frame[1] = function;               // 功能码（03-读保持寄存器）
    frame[2] = (start_addr >> 8) & 0xFF;  // 起始地址高8位
    frame[3] = start_addr & 0xFF;         // 起始地址低8位
    frame[4] = (reg_count >> 8) & 0xFF;   // 寄存器数量高8位
    frame[5] = reg_count & 0xFF;          // 寄存器数量低8位
    
    // 计算CRC16校验码
    crc = Modbus_CRC16(frame, 6);
    frame[6] = crc & 0xFF;            // CRC16低字节
    frame[7] = (crc >> 8) & 0xFF;     // CRC16高字节
    
    // 清空接收缓冲区
    ModbusMaster_ResetBuffer();
    
    // 发送数据（使用DMA发送）
    Serial_SendArray_2(frame, 8);
    
    return 1;
}


extern Task_Timer task_tim1;

// 修改接收响应帧函数为非阻塞方式
uint8_t ModbusMaster_ReceiveResponse(uint8_t *data, uint16_t *length, 
                                    uint32_t timeout_ms)
{
    static uint32_t start_time = 0;
    
    
// 如果没有传感器连接，立即返回超时，不等待
    // 可以添加一个传感器检测标志，这里简化处理直接返回超时
    static uint8_t sensor_detected = 0;
    
    // 如果是第一次调用，尝试检测传感器
    if(!sensor_detected) {
        // 简单检测：检查是否有任何串口数据
        if(rx_index_3 > 0) {
            sensor_detected = 1;
        } else {
            // 没有传感器，直接返回超时
            return MODBUS_MASTER_TIMEOUT;
        }
    }
    
    
    // 第一次调用时记录开始时间
    if(!modbus_receiving) {
        start_time = task_tim1.time_1ms;
        modbus_receive_start_time = start_time;
        modbus_receiving = 1;
    }
    
    // 检查是否超时
    if((task_tim1.time_1ms - start_time) > timeout_ms) {
        modbus_receiving = 0;
        return MODBUS_MASTER_TIMEOUT;
    }
    
    // 检查是否收到数据
    if(modbus_rx_complete) {
        // 复制数据到输出缓冲区
        *length = rx_index_3;
        memcpy(data, rx_buffer_3, rx_index_3);
        
        // 重置接收标志
        modbus_rx_complete = 0;
        rx_index_3 = 0;
        modbus_receiving = 0;
        
        return MODBUS_MASTER_OK;
    }
    
    // 返回忙碌状态，表示还在等待响应
    return MODBUS_MASTER_BUSY;
}

////// 接收响应帧
//uint8_t ModbusMaster_ReceiveResponse(uint8_t *data, uint16_t *length, 
//                                    uint32_t timeout_ms)
//{
//    uint32_t start_time = 0;
//    uint32_t timeout_counter = 0;
//    
//    // 简单延时等待响应（使用微秒延时）
//    for(timeout_counter = 0; timeout_counter < (timeout_ms * 1000); timeout_counter++) {
//        if(modbus_rx_complete) {
//            break;
//        }
//        Delay_us(1);  // 延时1微秒
//    }
//    
//    if(!modbus_rx_complete) {
//        return MODBUS_MASTER_TIMEOUT;
//    }
////    
////    // 复制数据到输出缓冲区
//    *length = rx_index_3;
//    memcpy(data, rx_buffer_3, rx_index_3);
//    
//    // 重置接收标志
//    modbus_rx_complete = 0;
//    
//    return MODBUS_MASTER_OK;
//}


//uint8_t ModbusMaster_ReceiveResponse(uint8_t *data, uint16_t *length 
//                                   )
//{
//    uint32_t start_time = 0;
////    
////    Serial_SendString_2("HTM440: 温度=");

////    
////    // 复制数据到输出缓冲区
//    *length = rx_index_3;
//    memcpy(data, rx_buffer_3, rx_index_3);
//    
//    // 重置接收标志
//    modbus_rx_complete = 0;
//    
//    return MODBUS_MASTER_OK;
//}



// 重置接收缓冲区
void ModbusMaster_ResetBuffer(void)
{
    rx_index_3 = 0;
    modbus_rx_complete = 0;
}

// 读取设备地址（广播地址00）
ModbusMaster_Status HTM440_Read_Device_Address(uint8_t *addr)
{
    uint8_t response[64];
    uint16_t response_len;
    uint8_t status;
    
    // 发送读取设备地址请求（广播地址00）
    ModbusMaster_SendRequest(0x00, MODBUS_READ_HOLDING_REG, 
                           DEVICE_ADDR_REG, 0x0001);
    
    // 等待响应
    status = ModbusMaster_ReceiveResponse(response, &response_len, 500);
    
//      status = ModbusMaster_ReceiveResponse(response, &response_len);
    
    
    if(status != MODBUS_MASTER_OK) {
        return status;
    }
    
    // 验证响应格式
    if(response_len < 5) {
        return MODBUS_MASTER_INVALID_RESPONSE;
    }
    
    // 验证CRC
    uint16_t received_crc = response[response_len-2] | (response[response_len-1] << 8);
    uint16_t calculated_crc = Modbus_CRC16(response, response_len-2);
    
    if(received_crc != calculated_crc) {
        return MODBUS_MASTER_CRC_ERROR;
    }
    
    // 解析设备地址（响应中的字节2和3）
    // 响应格式：地址 03 02 00 01 CRC16
    *addr = response[4];  // 设备地址在数据部分的低字节
    
    return MODBUS_MASTER_OK;
}




// 修改读取温湿度函数，使其能够处理非阻塞接收
ModbusMaster_Status HTM440_Read_Temperature_Humidity(uint8_t slave_addr, HTM440_Data_t *data)
{
    static uint8_t response[64];
    static uint16_t response_len;
    static ModbusMaster_Status status = MODBUS_MASTER_BUSY;
    static uint32_t read_start_time = 0;
    
    // 检查是否超时（500ms）
    if(read_start_time > 0 && (task_tim1.time_1ms - read_start_time) > 500) {
        data->read_status = MODBUS_MASTER_TIMEOUT;
        status = MODBUS_MASTER_BUSY; // 重置状态
        read_start_time = 0;
        return MODBUS_MASTER_TIMEOUT;
    }
    
    // 如果状态是忙碌，说明需要发送请求
    if(status == MODBUS_MASTER_BUSY) {
        // 发送读取温湿度请求（起始地址0x0002，数量2）
        ModbusMaster_SendRequest(slave_addr, MODBUS_READ_HOLDING_REG, 
                               TEMP_REG_ADDR, 0x0002);
        
        read_start_time = task_tim1.time_1ms;
        status = MODBUS_MASTER_WAITING;
        return MODBUS_MASTER_BUSY; // 表示请求已发送，等待响应
    }
    
    // 如果状态是等待，调用非阻塞接收
    if(status == MODBUS_MASTER_WAITING) {
        uint8_t receive_status;
        receive_status = ModbusMaster_ReceiveResponse(response, &response_len, 500);
        
        if(receive_status == MODBUS_MASTER_BUSY) {
            // 还在等待，返回忙碌状态
            return MODBUS_MASTER_BUSY;
        }
        else if(receive_status == MODBUS_MASTER_TIMEOUT) {
            // 超时
            data->read_status = MODBUS_MASTER_TIMEOUT;
            status = MODBUS_MASTER_BUSY;
            read_start_time = 0;
            return MODBUS_MASTER_TIMEOUT;
        }
        else if(receive_status == MODBUS_MASTER_OK) {
            // 接收到数据，处理响应
            if(response_len != 9) {
                data->read_status = MODBUS_MASTER_INVALID_RESPONSE;
                status = MODBUS_MASTER_BUSY;
                read_start_time = 0;
                return MODBUS_MASTER_INVALID_RESPONSE;
            }
            
            // 验证CRC
            uint16_t received_crc = response[7] | (response[8] << 8);
            uint16_t calculated_crc = Modbus_CRC16(response, 7);
            
            if(received_crc != calculated_crc) {
                data->read_status = MODBUS_MASTER_CRC_ERROR;
                status = MODBUS_MASTER_BUSY;
                read_start_time = 0;
                return MODBUS_MASTER_CRC_ERROR;
            }
            
            // 验证从站地址
            if(response[0] != slave_addr) {
                data->read_status = MODBUS_MASTER_INVALID_ADDRESS;
                status = MODBUS_MASTER_BUSY;
                read_start_time = 0;
                return MODBUS_MASTER_INVALID_ADDRESS;
            }
            
            // 解析温度数据
            int16_t raw_temp = (response[3] << 8) | response[4];
            data->raw_temperature = raw_temp;         // 保存原始值
            data->temperature = raw_temp / 10.0f;
            
            // 解析湿度数据
            int16_t raw_humidity = (response[5] << 8) | response[6];
            data->raw_humidity = raw_humidity;        // 保存原始值
            data->humidity = raw_humidity / 10.0f;
            
            // 更新状态
            data->device_addr = slave_addr;
            data->valid = 1;
            data->read_status = MODBUS_MASTER_OK;
            
            // 重置状态
            status = MODBUS_MASTER_BUSY;
            read_start_time = 0;
            
            return MODBUS_MASTER_OK;
        }
    }
    
    return MODBUS_MASTER_BUSY;
}


//// 读取温湿度（同时读取）
//ModbusMaster_Status HTM440_Read_Temperature_Humidity(uint8_t slave_addr, HTM440_Data_t *data)
//{
//    uint8_t response[64];
//    uint16_t response_len;
//    uint8_t status;
//    
//    // 发送读取温湿度请求（起始地址0x0002，数量2）
//    ModbusMaster_SendRequest(slave_addr, MODBUS_READ_HOLDING_REG, 
//                           TEMP_REG_ADDR, 0x0002);
//    
////    // 等待响应
//    status = ModbusMaster_ReceiveResponse(response, &response_len,500);
////    status = ModbusMaster_ReceiveResponse(response, &response_len);
//    
//    if(status != MODBUS_MASTER_OK) {
//        data->read_status = status;
//        return status;
//    }
//    
//    // 验证响应格式（应该为：地址+功能码+字节数+4字节数据+CRC）
//    if(response_len != 9) {  // 1+1+1+4+2=9字节
//        data->read_status = MODBUS_MASTER_INVALID_RESPONSE;
//        return MODBUS_MASTER_INVALID_RESPONSE;
//    }
//    
//    // 验证CRC
//    uint16_t received_crc = response[7] | (response[8] << 8);
//    uint16_t calculated_crc = Modbus_CRC16(response, 7);
//    
//    if(received_crc != calculated_crc) {
//        data->read_status = MODBUS_MASTER_CRC_ERROR;
//        return MODBUS_MASTER_CRC_ERROR;
//    }
//    
//    // 验证从站地址
//    if(response[0] != slave_addr) {
//        data->read_status = MODBUS_MASTER_INVALID_ADDRESS;
//        return MODBUS_MASTER_INVALID_ADDRESS;
//    }
//    
//    // 解析温度数据（有符号16位整数，实际值的10倍）
//    int16_t raw_temp = (response[3] << 8) | response[4];
//    data->temperature = raw_temp / 10.0f;
//    
//    // 解析湿度数据（有符号16位整数，实际值的10倍）
//    int16_t raw_humidity = (response[5] << 8) | response[6];
//    data->humidity = raw_humidity / 10.0f;
//    
//    // 更新状态（不设置last_read，由调用者设置）
//    data->device_addr = slave_addr;
//    data->valid = 1;
//    data->read_status = MODBUS_MASTER_OK;
//    
//    return MODBUS_MASTER_OK;
//}

// 只读取温度
ModbusMaster_Status HTM440_Read_Temperature(uint8_t slave_addr, HTM440_Data_t *data)
{
    uint8_t response[64];
    uint16_t response_len;
    uint8_t status;
    
    // 发送读取温度请求（起始地址0x0002，数量1）
    ModbusMaster_SendRequest(slave_addr, MODBUS_READ_HOLDING_REG, 
                           TEMP_REG_ADDR, 0x0001);
    
//    // 等待响应
    status = ModbusMaster_ReceiveResponse(response, &response_len, 500);
    
//    
//    status = ModbusMaster_ReceiveResponse(response, &response_len);
    
    if(status != MODBUS_MASTER_OK) {
        data->read_status = status;
        return status;
    }
    
    // 验证响应格式（应该为：地址+功能码+字节数+2字节数据+CRC）
    if(response_len != 7) {  // 1+1+1+2+2=7字节
        data->read_status = MODBUS_MASTER_INVALID_RESPONSE;
        return MODBUS_MASTER_INVALID_RESPONSE;
    }
    
    // 验证CRC
    uint16_t received_crc = response[5] | (response[6] << 8);
    uint16_t calculated_crc = Modbus_CRC16(response, 5);
    
    if(received_crc != calculated_crc) {
        data->read_status = MODBUS_MASTER_CRC_ERROR;
        return MODBUS_MASTER_CRC_ERROR;
    }
    
    // 验证从站地址
    if(response[0] != slave_addr) {
        data->read_status = MODBUS_MASTER_INVALID_ADDRESS;
        return MODBUS_MASTER_INVALID_ADDRESS;
    }
    
    // 解析温度数据
    int16_t raw_temp = (response[3] << 8) | response[4];
    data->temperature = raw_temp / 10.0f;
    data->humidity = 0.0f;  // 湿度未读取
    
    // 更新状态
    data->device_addr = slave_addr;
    data->valid = 1;
    data->read_status = MODBUS_MASTER_OK;
    
    return MODBUS_MASTER_OK;
}

// 只读取湿度
ModbusMaster_Status HTM440_Read_Humidity(uint8_t slave_addr, HTM440_Data_t *data)
{
    uint8_t response[64];
    uint16_t response_len;
    uint8_t status;
    
    // 发送读取湿度请求（起始地址0x0003，数量1）
    ModbusMaster_SendRequest(slave_addr, MODBUS_READ_HOLDING_REG, 
                           HUMIDITY_REG_ADDR, 0x0001);
    
//    // 等待响应
    status = ModbusMaster_ReceiveResponse(response, &response_len, 500);
    
//     status = ModbusMaster_ReceiveResponse(response, &response_len);
    
    
    if(status != MODBUS_MASTER_OK) {
        data->read_status = status;
        return status;
    }
    
    // 验证响应格式
    if(response_len != 7) {
        data->read_status = MODBUS_MASTER_INVALID_RESPONSE;
        return MODBUS_MASTER_INVALID_RESPONSE;
    }
    
    // 验证CRC
    uint16_t received_crc = response[5] | (response[6] << 8);
    uint16_t calculated_crc = Modbus_CRC16(response, 5);
    
    if(received_crc != calculated_crc) {
        data->read_status = MODBUS_MASTER_CRC_ERROR;
        return MODBUS_MASTER_CRC_ERROR;
    }
    
    // 验证从站地址
    if(response[0] != slave_addr) {
        data->read_status = MODBUS_MASTER_INVALID_ADDRESS;
        return MODBUS_MASTER_INVALID_ADDRESS;
    }
    
    // 解析湿度数据
    int16_t raw_humidity = (response[3] << 8) | response[4];
    data->humidity = raw_humidity / 10.0f;
    data->temperature = 0.0f;  // 温度未读取
    
    // 更新状态
    data->device_addr = slave_addr;
    data->valid = 1;
    data->read_status = MODBUS_MASTER_OK;
    
    return MODBUS_MASTER_OK;
}

// 初始化Modbus主站
void ModbusMaster_Init(void)
{
    // 初始化数据结构
    memset(&htm440_data, 0, sizeof(HTM440_Data_t));
    htm440_data.device_addr = MODBUS_SLAVE_ADDRESS;
}
