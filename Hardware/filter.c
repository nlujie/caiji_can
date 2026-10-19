#include "filter.h"
#include "stm32f10x.h"
#include <stdlib.h>
#include <math.h>

#define PI 3.1415926535f

/******************************************************************************************************************
*      medFilter(FilterStruct *filterStruct,short int newDatas,unsigned char MedNumSum,unsigned char MedNumGet)
*
*Description : 连续中值滤波算法
*Returns     : 当前的数据
*Notes       : 输入的数据在程序中完成缓存
*              
							*filterStruct		选择滤波的结构体
							newDatas				新数据
							MedNumSum				滤波样本总数
							MedNumGet				选择第几个数据输出
*******************************************************************************************************************
*/
float medFilter(FilterStruct *filterStruct,short int newDatas,int MedNumSum,unsigned char MedNumGet)
{
	unsigned char i,j;// 循环变量
	short int dataFilter[MedNumSum];
	short int dataTemp = 0;
	//存取数据
	filterStruct->data[filterStruct->storeNum++] = newDatas;
	if(filterStruct->storeNum >= MedNumSum)    
	{
		filterStruct->storeNum = 0;
	}
	
	for(i = 0;i < MedNumSum;i ++)   
	{
		dataFilter[i] = filterStruct->data[i];    //数据复制
	}
	
	// 用冒泡法对数组进行排序，从小到大排序
	for (j = 0; j < MedNumSum - 1; j ++)
	{
		for (i = 0; i < MedNumSum - j - 1; i ++)
		{
			if (dataFilter[i] > dataFilter[i + 1])
			{
				// 互换
				dataTemp = dataFilter[i];
				dataFilter[i] = dataFilter[i + 1];
				dataFilter[i + 1] = dataTemp;
			}
		}
	}
	return dataFilter[MedNumGet];
}


/******************************************************************************************************************
*        			  meanFilter(FilterStruct *filterStruct,short int newDatas,unsigned char MeanSum)
*
*Description : 滑动均值滤波算法
*Returns     : 当前的数据
*Notes       : 输入的数据在程序中完成缓存
*              
							*filterStruct		选择滤波的结构体
							newDatas				新数据
							MeanSum					滤波样本总数
*******************************************************************************************************************
*/
float meanFilter(FilterStruct *filterStruct,short int newDatas,unsigned char MeanSum)
{
	float dataSum = 0;
	unsigned char i = 0;
	
	filterStruct->data[filterStruct->storeNum++] = newDatas;  
	if(filterStruct->storeNum >= MeanSum)
	{
		filterStruct->storeNum = 0;
	}
	for(i = 0;i < MeanSum;i++)
	{
		dataSum += filterStruct->data[i];
	}
	return dataSum/MeanSum;
}

/******************************************************************************************************************
*          enhanceMeanFilter(FilterStruct *filterStruct,short int newDatas,unsigned char EnhanceMeanSum)
*
*Description : 增强型滑动均值滤波算法    去掉一个最大值和最小值
*Returns     : 当前的数据
*Notes       : 输入的数据在程序中完成缓存
*              
							*filterStruct		选择滤波的结构体
							newDatas			输入新数据
							EnhanceMeanSum		滤波样本总数
*******************************************************************************************************************
*/
float enhanceMeanFilter(FilterStruct *filterStruct,float newDatas,unsigned char EnhanceMeanSum)
{
	float dataSum = 0;
	unsigned char i = 0;
	static short int max,min;
	filterStruct->data[filterStruct->storeNum++] = newDatas;  
	if(filterStruct->storeNum >= EnhanceMeanSum)
	{
		filterStruct->storeNum = 0;
	}
	
	max = min = filterStruct->data[0];    //初始化
	//找到最大最小值
	for(i =1;i < EnhanceMeanSum; i ++)
	{
		if(filterStruct->data[i] > max)
		{
			max = filterStruct->data[i];
		}
		
		if(filterStruct->data[i] < min)
		{
			min = filterStruct->data[i];
		}
	}
	
	for(i = 0;i < EnhanceMeanSum;i ++)
	{
		dataSum += filterStruct->data[i];
	}
	dataSum -= (max + min); 
	return dataSum/(EnhanceMeanSum-2);
}

//float enhanceMeanFilter_float(FilterStruct_Float *filterStruct_Float,float newDatas,unsigned char EnhanceMeanSum)
//{
//	float dataSum = 0;
//	unsigned char i = 0;
//	static short int max,min;
//	filterStruct_Float->data[filterStruct_Float->storeNum++] = newDatas;  
//	if(filterStruct_Float->storeNum >= EnhanceMeanSum)
//	{
//		filterStruct_Float->storeNum = 0;
//	}
//	
//	max = min = filterStruct_Float->data[0];    //初始化
//	//找到最大最小值
//	for(i =1;i < EnhanceMeanSum; i ++)
//	{
//		if(filterStruct_Float->data[i] > max)
//		{
//			max = filterStruct_Float->data[i];
//		}
//		
//		if(filterStruct_Float->data[i] < min)
//		{
//			min = filterStruct_Float->data[i];
//		}
//	}
//	
//	for(i = 0;i < EnhanceMeanSum;i ++)
//	{
//		dataSum += filterStruct_Float->data[i];
//	}
//	dataSum -= (max + min); 
//	return dataSum/(EnhanceMeanSum-2);
//}

float enhanceMeanFilter_float(FilterStruct_Float *filterStruct_Float, float newDatas, unsigned char EnhanceMeanSum) {
    if (filterStruct_Float->data == NULL || EnhanceMeanSum == 0) {
        return newDatas;
    }
    
    float dataSum = 0;
    unsigned char i = 0;
    
    // 存储新数据
    filterStruct_Float->data[filterStruct_Float->storeNum++] = newDatas;  
    if(filterStruct_Float->storeNum >= EnhanceMeanSum) {
        filterStruct_Float->storeNum = 0;
    }
    
    // 找到最大最小值
    float max = filterStruct_Float->data[0];
    float min = filterStruct_Float->data[0];
    
    for(i = 1; i < EnhanceMeanSum; i++) {
        if(filterStruct_Float->data[i] > max) {
            max = filterStruct_Float->data[i];
        }
        if(filterStruct_Float->data[i] < min) {
            min = filterStruct_Float->data[i];
        }
    }
    
    // 计算总和（去掉最大最小值）
    for(i = 0; i < EnhanceMeanSum; i++) {
        dataSum += filterStruct_Float->data[i];
    }
    dataSum -= (max + min); 
    
    return dataSum / (EnhanceMeanSum - 2);
}




/*
// 简化版固定工频陷波滤波器
typedef struct {
    float b0, b1, b2;
    float a1, a2;
    float x1, x2;
    float y1, y2;
} SimpleNotchFilter;
*/
// 初始化双工频陷波滤波器
void init_dual_notch(SimpleNotchFilter* filter, float fs) {
    // 设计同时抑制50Hz和60Hz的宽陷波
    float f0 = 50.0f; // 中心频率
    float BW = 1.0f;  // 带宽20Hz（覆盖50-60Hz）
    float Q = f0/BW;   // 品质因数
    
    float w0 = 2 * PI * f0 / fs;
    float alpha = sinf(w0) / (2 * Q);
    
    filter->b0 = 1.0f;
    filter->b1 = -2.0f * cosf(w0);
    filter->b2 = 1.0f;
    float a0 = 1.0f + alpha;
    filter->a1 = -2.0f * cosf(w0) / a0;
    filter->a2 = (1.0f - alpha) / a0;
    
    // 初始化状态
    filter->x1 = filter->x2 = 0.0f;
    filter->y1 = filter->y2 = 0.0f;
}

// 应用固定陷波滤波器
float apply_simple_notch(SimpleNotchFilter* filter, float input) {
    float output = filter->b0 * input + filter->b1 * filter->x1 + filter->b2 * filter->x2
                   - filter->a1 * filter->y1 - filter->a2 * filter->y2;
    
    // 更新状态
    filter->x2 = filter->x1;
    filter->x1 = input;
    filter->y2 = filter->y1;
    filter->y1 = output;
    
    return output;
}

// 简化的自适应机制（仅开关控制）
void adaptive_control(SimpleNotchFilter* filter, float* buffer, int size) {
    // 计算信号能量
    float energy = 0.0f;
    for(int i = 0; i < size; i++) {
        energy += buffer[i] * buffer[i];
    }
    energy /= size;
    
    // 仅当检测到强工频干扰时启用陷波
    static float threshold = 0.0001f; // 0.1mV²
    filter->b0 = (energy > threshold) ? 1.0f : 0.0f;
}


//// 初始化复合滤波器
//void init_dual_filter(DualStageFilter* filter, float fs, int mean_window) {
//    // 初始化均值滤波
//    filter->mean_filter.data = (float*)malloc(mean_window * sizeof(float));
//    filter->mean_filter.maxSize = mean_window;
//    filter->mean_filter.storeNum = 0;
//    for(int i=0; i<mean_window; i++) {
//        filter->mean_filter.data[i] = 0.0f;
//    }
//    
//    // 初始化陷波滤波
//    init_dual_notch(&filter->notch_filter, fs);
//    
//    // 初始化原始缓存
//    filter->buf_index = 0;
//    for(int i=0; i<5; i++) {
//        filter->raw_buffer[i] = 0.0f;
//    }
//}

// 应用复合滤波
float apply_dual_filter(DualStageFilter* filter, float input) {
    // 1. 存储原始值（用于自适应控制）
    filter->raw_buffer[filter->buf_index] = input;
    filter->buf_index = (filter->buf_index + 1) % 5;
    
    // 2. 增强型均值滤波（去异常值）
    float mean_filtered = enhanceMeanFilter(&filter->mean_filter, input, filter->mean_filter.maxSize);
    
    // 3. 定期执行自适应控制（每5个样本）
    if(filter->buf_index == 0) {
        adaptive_control(&filter->notch_filter, filter->raw_buffer, 5);
    }
    
    // 4. 应用陷波滤波
    return apply_simple_notch(&filter->notch_filter, mean_filtered);
}




//// 初始化复合滤波器
//void init_composite_filter(CompositeFilter* filter, float fs, int mean_window) {
//    // 初始化均值滤波
//    filter->mean_filter.data = (float*)malloc(mean_window * sizeof(float));
//    filter->mean_filter.maxSize = mean_window;
//    filter->mean_filter.storeNum = 0;
//    for(int i=0; i<mean_window; i++) {
//        filter->mean_filter.data[i] = 0.0f;
//    }
//    
//    // 初始化陷波滤波
//    init_dual_notch(&filter->notch_filter, fs);
//    
//    // 初始化原始缓存
//    filter->buf_index = 0;
//    filter->sample_count = 0;
//    for(int i=0; i<10; i++) {
//        filter->raw_buffer[i] = 0.0f;
//    }
//}




// 初始化复合滤波器（带错误检查）
 int init_composite_filter(CompositeFilter* filter, float fs, int mean_window) {
    // 初始化均值滤波 - 动态分配内存
    filter->mean_filter.data = (float*)malloc(mean_window * sizeof(float));
    if (filter->mean_filter.data == NULL) {
        return 0; // 内存分配失败
    }
    
    filter->mean_filter.maxSize = mean_window;
    filter->mean_filter.storeNum = 0;
    
    // 初始化数据
    for(int i = 0; i < mean_window; i++) {
        filter->mean_filter.data[i] = 0.0f;
    }
    
    // 初始化陷波滤波（根据实际采样率调整参数）
    init_dual_notch(&filter->notch_filter, fs);
    
    // 初始化原始缓存和其他状态
    filter->buf_index = 0;
    filter->sample_count = 0;
    filter->last_filtered_value = 0.0f;
    
    for(int i = 0; i < 10; i++) {
        filter->raw_buffer[i] = 0.0f;
    }
    
    return 1; // 初始化成功
}


//// 初始化复合滤波器
//void init_composite_filter(CompositeFilter* filter, float fs, int mean_window) {
//    // 初始化均值滤波 - 动态分配内存
//    filter->mean_filter.data = (float*)malloc(mean_window * sizeof(float));
//    if (filter->mean_filter.data != NULL) {
//        filter->mean_filter.maxSize = mean_window;
//        filter->mean_filter.storeNum = 0;
//        for(int i = 0; i < mean_window; i++) {
//            filter->mean_filter.data[i] = 0.0f;
//        }
//    }
//    
//    // 初始化陷波滤波
//    init_dual_notch(&filter->notch_filter, fs);
//    
//    // 初始化原始缓存
//    filter->buf_index = 0;
//    filter->sample_count = 0;
//    for(int i = 0; i < 10; i++) {
//        filter->raw_buffer[i] = 0.0f;
//    }
//}

// 初始化双级滤波器
void init_dual_filter(DualStageFilter* filter, float fs, int mean_window) {
    // 初始化均值滤波 - 动态分配内存
    filter->mean_filter.data = (float*)malloc(mean_window * sizeof(float));
    if (filter->mean_filter.data != NULL) {
        filter->mean_filter.maxSize = mean_window;
        filter->mean_filter.storeNum = 0;
        for(int i = 0; i < mean_window; i++) {
            filter->mean_filter.data[i] = 0.0f;
        }
    }
    
    // 初始化陷波滤波
    init_dual_notch(&filter->notch_filter, fs);
    
    // 初始化原始缓存
    filter->buf_index = 0;
    for(int i = 0; i < 5; i++) {
        filter->raw_buffer[i] = 0.0f;
    }
}









//// 应用复合滤波，每10个数据滤波一次
//float apply_composite_filter(CompositeFilter* filter, float input) {
//    // 存储原始值
//    filter->raw_buffer[filter->buf_index] = input;
//    filter->buf_index = (filter->buf_index + 1) % 10;
//    filter->sample_count++;
//    
//    // 每10个数据执行一次复合滤波
//    if(filter->sample_count >= 10) {
//        filter->sample_count = 0;
//        
//        // 1. 对10个原始数据执行增强型均值滤波
//        float mean_filtered = 0.0f;
//        float sum = 0.0f;
//        float min_val = filter->raw_buffer[0];
//        float max_val = filter->raw_buffer[0];
//        
//        // 计算总和并找到最大最小值
//        for(int i = 0; i < 10; i++) {
//            sum += filter->raw_buffer[i];
//            if(filter->raw_buffer[i] < min_val) min_val = filter->raw_buffer[i];
//            if(filter->raw_buffer[i] > max_val) max_val = filter->raw_buffer[i];
//        }
//        
//        // 去掉最大值和最小值后求平均
//        mean_filtered = (sum - min_val - max_val) / 8.0f;
//        
//        // 2. 对均值滤波结果应用陷波滤波
//        float final_filtered = apply_simple_notch(&filter->notch_filter, mean_filtered);
//        
//        return final_filtered;
//    }
//    
//    // 如果不满10个数据，返回原始值或0（根据需求选择）
//    return input; // 或者返回0，表示数据不足
//}


// 改进的复合滤波，每10个数据输出一个滤波值，但保持连续性
//float apply_composite_filter(CompositeFilter* filter, float input) {
//    // 存储原始值
//    filter->raw_buffer[filter->buf_index] = input;
//    filter->buf_index = (filter->buf_index + 1) % 10;
//    filter->sample_count++;
//    
//    // 每10个数据执行一次复合滤波
//    if(filter->sample_count >= 10) {
//        // 1. 对10个原始数据执行增强型均值滤波
//        float mean_filtered = 0.0f;
//        float sum = 0.0f;
//        float min_val = filter->raw_buffer[0];
//        float max_val = filter->raw_buffer[0];
//        
//        // 计算总和并找到最大最小值
//        for(int i = 0; i < 10; i++) {
//            sum += filter->raw_buffer[i];
//            if(filter->raw_buffer[i] < min_val) min_val = filter->raw_buffer[i];
//            if(filter->raw_buffer[i] > max_val) max_val = filter->raw_buffer[i];
//        }
//        
//        // 去掉最大值和最小值后求平均
//        mean_filtered = (sum - min_val - max_val) / 8.0f;
//        
//        
//        
//       
////        // 2. 对均值滤波结果应用陷波滤波
////        float final_filtered = apply_simple_notch(&filter->notch_filter, mean_filtered);
////      
////        
////        
////        return final_filtered;
//       return mean_filtered;
//    }
//    
//    // 如果不满10个数据，返回上一次的滤波值或进行简单滤波
//    static float last_filtered_value = 0;
//    if(filter->sample_count == 1) {
//        // 第一个数据，直接返回
//        last_filtered_value = input;
//    } else {
//        // 对已有数据进行简单移动平均
//        float temp_sum = 0;
//        for(int i = 0; i < filter->sample_count; i++) {
//            temp_sum += filter->raw_buffer[i];
//        }
//        last_filtered_value = temp_sum / filter->sample_count;
//    }
//    
//    return last_filtered_value;
//}



// 应用复合滤波，每10个数据滤波一次  温度专用
float apply_composite_filter1(CompositeFilter* filter, float input) {
    // 检查滤波器是否初始化成功
    if (filter->mean_filter.data == NULL) {
        return input; // 如果初始化失败，直接返回原始值
    }
    
    // 存储原始值
    filter->raw_buffer[filter->buf_index] = input;
    filter->buf_index = (filter->buf_index + 1) % 10;
    filter->sample_count++;
    
    // 每10个数据执行一次复合滤波
    if(filter->sample_count >= 10) {
        // 1. 对10个原始数据执行增强型均值滤波
        float mean_filtered = 0.0f;
        float sum = 0.0f;
        float min_val = filter->raw_buffer[0];
        float max_val = filter->raw_buffer[0];
        
        // 计算总和并找到最大最小值
        for(int i = 0; i < 10; i++) {
            sum += filter->raw_buffer[i];
            if(filter->raw_buffer[i] < min_val) min_val = filter->raw_buffer[i];
            if(filter->raw_buffer[i] > max_val) max_val = filter->raw_buffer[i];
        }
        
        // 去掉最大值和最小值后求平均
        mean_filtered = (sum - min_val - max_val) / 8.0f;
        
//        // 2. 对均值滤波结果应用陷波滤波
//        float final_filtered = apply_simple_notch(&filter->notch_filter, mean_filtered);
//        
//        // 更新上一次的滤波值（使用结构体成员，避免静态变量）
//        filter->last_filtered_value = final_filtered;
        
        
        
//        return final_filtered;
        
        
        
        //        // 更新上一次的滤波值（使用结构体成员，避免静态变量）
        filter->last_filtered_value = mean_filtered;
        return mean_filtered;
    }
    
    // 如果不满10个数据，使用结构体成员保持连续性
    if(filter->sample_count == 1) {
        // 第一个数据，直接返回并保存
        filter->last_filtered_value = input;
    } else {
        // 对已有数据进行简单移动平均
        float temp_sum = 0;
        int valid_count = (filter->sample_count < 10) ? filter->sample_count : 10;
        for(int i = 0; i < valid_count; i++) {
            temp_sum += filter->raw_buffer[i];
        }
        filter->last_filtered_value = temp_sum / valid_count;
    }
    
    return filter->last_filtered_value;
}


// 应用复合滤波，每10个数据滤波一次
float apply_composite_filter(CompositeFilter* filter, float input) {
    // 检查滤波器是否初始化成功
    if (filter->mean_filter.data == NULL) {
        return input; // 如果初始化失败，直接返回原始值
    }
    
    // 存储原始值
    filter->raw_buffer[filter->buf_index] = input;
    filter->buf_index = (filter->buf_index + 1) % 10;
    filter->sample_count++;
    
    // 每10个数据执行一次复合滤波
    if(filter->sample_count >= 10) {
        // 1. 对10个原始数据执行增强型均值滤波
        float mean_filtered = 0.0f;
        float sum = 0.0f;
        float min_val = filter->raw_buffer[0];
        float max_val = filter->raw_buffer[0];
        
        // 计算总和并找到最大最小值
        for(int i = 0; i < 10; i++) {
            sum += filter->raw_buffer[i];
            if(filter->raw_buffer[i] < min_val) min_val = filter->raw_buffer[i];
            if(filter->raw_buffer[i] > max_val) max_val = filter->raw_buffer[i];
        }
        
        // 去掉最大值和最小值后求平均
        mean_filtered = (sum - min_val - max_val) / 8.0f;
        
//        // 2. 对均值滤波结果应用陷波滤波
//        float final_filtered = apply_simple_notch(&filter->notch_filter, mean_filtered);
//        
//        // 更新上一次的滤波值（使用结构体成员，避免静态变量）
//        filter->last_filtered_value = final_filtered;
//        
        filter->last_filtered_value = mean_filtered;
//        
//        return final_filtered;
        
       return mean_filtered;
        
    }
    
    // 如果不满10个数据，使用结构体成员保持连续性
    if(filter->sample_count == 1) {
        // 第一个数据，直接返回并保存
        filter->last_filtered_value = input;
    } else {
        // 对已有数据进行简单移动平均
        float temp_sum = 0;
        int valid_count = (filter->sample_count < 10) ? filter->sample_count : 10;
        for(int i = 0; i < valid_count; i++) {
            temp_sum += filter->raw_buffer[i];
        }
        filter->last_filtered_value = temp_sum / valid_count;
    }
    
    return filter->last_filtered_value;
}



/******************************************************************************************************************
*             Average_Filter(Filter_Struct *filterStruct,short int newDatas,unsigned char FilterNum)
*
*Description : 均值滤波
*Returns     : 当前的数据
*Notes       : 输入的数据在程序中完成缓存
*              
							*filterStruct		选择滤波的结构体
							newDatas				新数据
							FilterNum				滤波样本总数
							*Out						输出值
*******************************************************************************************************************
*/
void Average_Filter(Filter_Struct *filterStruct,short int newDatas,unsigned char FilterNum,uint32_t *Out)
{
	unsigned int i = 0;
	float FilterSum  = 0;
	
	filterStruct->Data_His[filterStruct->Count++] = newDatas;
	
	if( filterStruct->Complete )                //已经采集完最初的AdcFilterNum个数据
	{
		for(i = 0; i < FilterNum; i++)
		{
			FilterSum += filterStruct->Data_His[i];
		}
		filterStruct->Cache = FilterSum / (FilterNum);
	}
	
	if(filterStruct->Complete  == 1)
	{
		if(filterStruct->Cache < 5000)
		{
			*Out 	 = filterStruct->Cache;
			filterStruct->Last_Data 	 = filterStruct->Cache;
		}
		else
			*Out = filterStruct->Last_Data;
	}
	if(filterStruct->Count == FilterNum)
	{
		filterStruct->Complete   = 1;
		filterStruct->Count = 0;
	}
}


/******************************************************************************************************************
*             Enaverage_Filter(Filter_Struct *filterStruct,short int newDatas,unsigned char FilterNum)
*
*Description : 增强型均值滤波  去掉一个最大值和最小值
*Returns     : 当前的数据
*Notes       : 输入的数据在程序中完成缓存
*              
							*filterStruct		选择滤波的结构体
							newDatas				新数据
							FilterNum				滤波样本总数
							*Out						输出值
*******************************************************************************************************************
*/
void Enaverage_Filter(Filter_Struct *filterStruct,short int newDatas,unsigned char FilterNum,short int *Out)
{
	unsigned int i = 0;
	float FilterSum  = 0;
	static unsigned short int max,min;
	
	filterStruct->Data_His[filterStruct->Count++] = newDatas;
	
	if( filterStruct->Complete )                //已经采集完最初的AdcFilterNum个数据
	{
		max = min = filterStruct->Data_His[0];    //初始化
	//找到最大最小值
		for(i =1;i < FilterNum; i ++)
		{
			if(filterStruct->Data_His[i] > max)
			{
				max = filterStruct->Data_His[i];
			}
			
			if(filterStruct->Data_His[i] < min)
			{
				min = filterStruct->Data_His[i];
			}
		}
		
		for(i = 0; i < FilterNum; i++)
		{
			FilterSum += filterStruct->Data_His[i];
		}
		FilterSum -= max + min;
		filterStruct->Cache = FilterSum / (FilterNum-2);
	}
	
	if(filterStruct->Complete  == 1)
	{
		if(filterStruct->Cache < 5000)
		{
			*Out = filterStruct->Cache;
			filterStruct->Last_Data 	 = filterStruct->Cache;
		}
		else
			*Out = filterStruct->Last_Data;
	}
	if(filterStruct->Count == FilterNum)
	{
		filterStruct->Complete   = 1;
		filterStruct->Count = 0;
	}
}

/******************************************************************************************************************
*        			  meanFilter_Float(FilterStruct *filterStruct,short int newDatas,unsigned char MeanSum)
*
*Description : 滑动均值滤波算法(输入为浮点型)
*Returns     : 当前的数据
*Notes       : 输入的数据在程序中完成缓存
*              
							*filterStruct		选择滤波的结构体
							newDatas				新数据
							MeanSum					滤波样本总数
*******************************************************************************************************************
*/
float meanFilter_Float(FilterStruct_Float *filterStruct,float newDatas,unsigned char MeanSum)
{
	float dataSum = 0;
	unsigned char i = 0;
	filterStruct->data[filterStruct->storeNum++] = newDatas;  
	if(filterStruct->storeNum >= MeanSum)
	{
		filterStruct->storeNum = 0;
	}
	
	for(i = 0;i < MeanSum;i++)
	{
		dataSum += filterStruct->data[i];
	}
	return dataSum/MeanSum;
}
