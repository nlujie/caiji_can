#ifndef __FILTER_H
#define __FILTER_H

#include "stm32f10x.h" 

// 修改 FilterStruct 定义，添加 maxSize 字段
typedef struct
{
    float *data;                    // 动态分配的数据指针
    unsigned char storeNum;         // 当前存储位置
    unsigned char maxSize;          // 缓冲区大小
} FilterStruct;

// 修改 FilterStruct_Float 定义，添加 maxSize 字段
typedef struct
{
    float *data;                    // 动态分配的数据指针
    unsigned char storeNum;         // 当前存储位置
    unsigned char maxSize;          // 缓冲区大小
} FilterStruct_Float;

// 简化自适应陷波结构
typedef struct {
    float b0, b1, b2;
    float a1, a2;
    float x1, x2;
    float y1, y2;
} SimpleNotchFilter;

// 双级滤波结构
typedef struct {
    FilterStruct mean_filter;       // 第一级：均值滤波
    SimpleNotchFilter notch_filter; // 第二级：陷波滤波
    float raw_buffer[5];            // 原始值缓存
    int buf_index;                  // 缓冲区索引
} DualStageFilter;

//// 复合滤波结构体
//typedef struct {
//    FilterStruct_Float mean_filter; // 均值滤波
//    SimpleNotchFilter notch_filter; // 陷波滤波
//    float raw_buffer[10];           // 原始数据缓冲区
//    int buf_index;                  // 缓冲区索引
//    int sample_count;               // 采样计数
//} CompositeFilter;


// 复合滤波结构体
typedef struct {
    FilterStruct_Float mean_filter;  // 均值滤波
    SimpleNotchFilter notch_filter;  // 陷波滤波
    float raw_buffer[10];            // 原始数据缓冲区
    int buf_index;                   // 缓冲区索引
    int sample_count;                // 采样计数
    float last_filtered_value;       // 上一次滤波值（避免使用静态变量）
} CompositeFilter;

// 传统滤波结构体
typedef struct
{
    unsigned char Complete;
    unsigned char Count;
    short int Cache;                // 计算中间缓存值
    short int Last_Data;            // 上一次捕获值
    short int Data_His[15];         // 数据历史值
} Filter_Struct;

// 函数声明
int init_composite_filter(CompositeFilter* filter, float fs, int mean_window);

float apply_composite_filter1(CompositeFilter* filter, float input);
float apply_composite_filter(CompositeFilter* filter, float input);

void init_dual_filter(DualStageFilter* filter, float fs, int mean_window);
float apply_dual_filter(DualStageFilter* filter, float input);
float enhanceMeanFilter_float(FilterStruct_Float *filterStruct_Float, float newDatas, unsigned char EnhanceMeanSum);
float medFilter(FilterStruct *filterStruct, short int newDatas, int MedNumSum, unsigned char MedNumGet);
float meanFilter(FilterStruct *filterStruct, short int newDatas, unsigned char MeanSum);
float enhanceMeanFilter(FilterStruct *filterStruct, float newDatas, unsigned char EnhanceMeanSum);
float meanFilter_Float(FilterStruct_Float *filterStruct, float newDatas, unsigned char MeanSum);
void Average_Filter(Filter_Struct *filterStruct, short int newDatas, unsigned char FilterNum, uint32_t *Out);
void Enaverage_Filter(Filter_Struct *filterStruct, short int newDatas, unsigned char FilterNum, short int *Out);
void init_dual_notch(SimpleNotchFilter* filter, float fs);
float apply_simple_notch(SimpleNotchFilter* filter, float input);

#endif
