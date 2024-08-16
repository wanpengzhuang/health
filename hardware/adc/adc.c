/*!
  \file	 	adc.c
  \brief	ADC配置
			**软件触发：CPU控制触发，在while（1）中控制，灵活但有延迟且主循环延时
			**定时器硬件触发：ADC硬件触发器控制，实时性高，中断服务函数中进行
			**DMA模式
  \author	chuang
  
  \version	2024-05-16,for GD32F303CC 
  \modify	2024-05-16
			**增加硬件定时器触发版本
			不用宏定义，定时器1通道3，ADC0通道3，软件触发可以，定时器触发不行
		    尝试不使用该引脚上的定时器，使用定时器1的通道0（按照示例使用插入通道）成功
		    定时器还需要调整，定时0.02秒会停止
			  
			**增加DMA模式todo			 
*/

/************************头文件**********************************/
#include "adc.h"


//PPG***********
#define PPG_BOARD_ADC_CHANNEL   ADC_CHANNEL_4//ADC_CHANNEL_13
#define PPG_PPG_ADC_GPIO_PORT_RCU   RCU_GPIOA//RCU_GPIOC
#define PPG_ADC_GPIO_PORT       GPIOA//GPIOC
#define PPG_ADC_GPIO_PIN        GPIO_PIN_4//GPIO_PIN_3


/************************函数定义**********************************/
void rcu_config(void)
{
    //启动GPIO时钟
    rcu_periph_clock_enable(RCU_GPIOA);
	
    //启动ADC时钟,压电
    rcu_periph_clock_enable(RCU_ADC0);
	
    //配置ADC时钟，ADC时钟频率选择CK_APB2时钟频率为原频率的1/4
    rcu_adc_clock_config(RCU_CKADC_CKAPB2_DIV4);
	
}

void adc_gpio_config(void)
{
    //配置端口模式为模拟输入，端口速度为最大速度
    gpio_init(GPIOA, GPIO_MODE_AIN, GPIO_OSPEED_MAX, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);
	
}

void adc_config(void)
{
	//ADC模式配置，自由模式代表可以在扫描和连续模式下工作
    adc_mode_config(ADC_MODE_FREE);
	
	// 设置ADC的连续转换模式（自动连续在当前通道转换）和扫描模式（自动切换通道转换）。
    adc_special_function_config(ADC0, ADC_CONTINUOUS_MODE, DISABLE);
    adc_special_function_config(ADC0, ADC_SCAN_MODE, ENABLE); 
	
	//配置ADC采样精度12位
	//adc_resolution_config(ADC0, ADC_RESOLUTION_12B);
	
	//ADC数据对齐配置
    adc_data_alignment_config(ADC0, ADC_DATAALIGN_RIGHT);
	
    //ADC通道长度配置，同时处理的通道数，多通道可以同时检测多传感器数据
    //adc_channel_length_config(ADC0, ADC_REGULAR_CHANNEL, 1);
	adc_channel_length_config(ADC0, ADC_INSERTED_CHANNEL, 4);
	
	//配置ADC的插入通道，多通道读取同一引脚提高效率
	adc_inserted_channel_config(ADC0, 0, ADC_CHANNEL_0, ADC_SAMPLETIME_55POINT5);
    adc_inserted_channel_config(ADC0, 1, ADC_CHANNEL_1, ADC_SAMPLETIME_55POINT5);
	adc_inserted_channel_config(ADC0, 2, ADC_CHANNEL_2, ADC_SAMPLETIME_55POINT5);
	adc_inserted_channel_config(ADC0, 3, ADC_CHANNEL_3, ADC_SAMPLETIME_55POINT5);
    
	// 配置ADC的常规通道，采样时间也会影响得到的ADC数据，不要搞错
    //adc_regular_channel_config(ADC0, 0, BOARD_ADC_CHANNEL, ADC_SAMPLETIME_55POINT5);
	
	//采样时间55.5微秒
    //ADC触发配置，定时器1的第3通道触发，ADC0_1_2_EXTTRIG_REGULAR_NONE表示软件触发
    //adc_external_trigger_source_config(ADC0, ADC_REGULAR_CHANNEL, ADC0_1_EXTTRIG_REGULAR_T1_CH3); 
	//adc_external_trigger_source_config(ADC0, ADC_REGULAR_CHANNEL, ADC0_1_2_EXTTRIG_REGULAR_NONE); 
	//ADC触发配置，定时器1的第0通道触发,timer.c里面配置AD硬件触发定时器，不必是当前引脚上的定时器
	adc_external_trigger_source_config(ADC0, ADC_INSERTED_CHANNEL, ADC0_1_EXTTRIG_INSERTED_T1_CH0);
	
	//ADC外部触发配置
    //adc_external_trigger_config(ADC0, ADC_REGULAR_CHANNEL, ENABLE);
	adc_external_trigger_config(ADC0, ADC_INSERTED_CHANNEL, ENABLE);
	
	// 设置ADC的过采样模式和比例。
    adc_oversample_mode_config(ADC0, ADC_OVERSAMPLING_ALL_CONVERT, ADC_OVERSAMPLING_SHIFT_4B, ADC_OVERSAMPLING_RATIO_MUL16);
    adc_oversample_mode_enable(ADC0);
	
	//清除ADC的EOC结束转换标志和EOIC结束插入标志,不使用插入通道时不需要下面代码
    adc_interrupt_flag_clear(ADC0, ADC_INT_FLAG_EOC);
    adc_interrupt_flag_clear(ADC0, ADC_INT_FLAG_EOIC);
    adc_interrupt_enable(ADC0, ADC_INT_EOIC);//启动EOIC中断
	
    //启动ADC接口
    adc_enable(ADC0);
    delay_1ms(1);
	//vTaskDelay(1);
	
    //ADC校准和复位校准
    adc_calibration_enable(ADC0);
	
	//配置NVIC
	//nvic_priority_group_set(NVIC_PRIGROUP_PRE1_SUB3);
	
	//RTOS中只能用第四组
	nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);
	
	//使用ADC0或ADC1，都可以使用ADC0_1_IRQn中断请求号
	nvic_irq_enable(ADC0_1_IRQn, 0, 0);
	
}

// 中断服务程序，不需要声明，会直接覆盖
void ADC0_1_IRQHandler(void)
{
	
    //static uint16_t adcValue; // 静态变量存储ADC数据
	
	// 等待ADC的转换完成，注释掉，中毒服务函数时间要短
	 //while(SET != adc_flag_get(ADC, ADC_FLAG_EOC));
	if(SET == adc_flag_get(ADC, ADC_FLAG_EOC))
	{
		// 读取ADC数据
		//adcValue = adc_regular_data_read(ADC0);//常规通道
		g_adc_data = adc_inserted_data_read(ADC0 , ADC_INSERTED_CHANNEL_1);//插入通道

		// 打印ADC数据
		//printf("pressure=%d\n", g_adc_data);

		// 清除中断标志，以便下一次中断时可以再次触发，现在不是用这个清除，因为用了插入通道
		//adc_flag_clear(ADC0, ADC_FLAG_EOC);
		
		//清除ADC的EOC结束转换标志和EOIC结束插入标志,不使用插入通道时不需要下面代码
		adc_interrupt_flag_clear(ADC0, ADC_INT_FLAG_EOC);
		adc_interrupt_flag_clear(ADC0, ADC_INT_FLAG_EOIC);
		
		//计算时间
		heart_time += 80;
		breath_time +=80;
		
		flag_get_new_adc_data = 1;
	}
}

//PPG**********************
void ppg_adc_config(void)
{
	// 启动GPIOA的时钟。
    rcu_periph_clock_enable(PPG_PPG_ADC_GPIO_PORT_RCU);
    // 启动ADC1的时钟。
    rcu_periph_clock_enable(RCU_ADC1);
    // 配置ADC的时钟。
    rcu_adc_clock_config(RCU_CKADC_CKAPB2_DIV4);
	
	 // 将GPIO设置为模拟输入模式。
    gpio_init(PPG_ADC_GPIO_PORT, GPIO_MODE_AIN, GPIO_OSPEED_MAX, PPG_ADC_GPIO_PIN);
	
    // 设置ADC的连续转换模式和扫描模式。
    adc_special_function_config(ADC1, ADC_CONTINUOUS_MODE, ENABLE);
    adc_special_function_config(ADC1, ADC_SCAN_MODE, DISABLE);  
    // 配置ADC的触发源。
    adc_external_trigger_source_config(ADC1, ADC_REGULAR_CHANNEL, ADC0_1_2_EXTTRIG_REGULAR_NONE); 
    // 配置ADC的数据对齐方式。
    adc_data_alignment_config(ADC1, ADC_DATAALIGN_RIGHT);
    // 配置ADC的工作模式。
    adc_mode_config(ADC_MODE_FREE); 
    // 配置ADC的通道长度。
    adc_channel_length_config(ADC1, ADC_REGULAR_CHANNEL, 1);
 
    // 配置ADC的常规通道。采样时间也会影响得到的ADC数据，不要搞错
    adc_regular_channel_config(ADC1, 0, PPG_BOARD_ADC_CHANNEL, ADC_SAMPLETIME_239POINT5);
    adc_external_trigger_config(ADC1, ADC_REGULAR_CHANNEL, ENABLE);
  
    // 设置ADC的过采样模式和比例。
    adc_oversample_mode_config(ADC1, ADC_OVERSAMPLING_ALL_CONVERT, ADC_OVERSAMPLING_SHIFT_4B, ADC_OVERSAMPLING_RATIO_MUL16);
    adc_oversample_mode_enable(ADC1);
  
    // 启动ADC。
    adc_enable(ADC1);
    delay_ms(1);
    // 启动ADC的校准。
    adc_calibration_enable(ADC1);
	
	//RTOS中只能用第四组
	nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);
	
	//使用ADC0或ADC1，都可以使用ADC0_1_IRQn中断请求号
	nvic_irq_enable(ADC0_1_IRQn, 0, 0);
}


/*
//软件触发测试用例
void adc_config(void)
{
	// 启动GPIOA的时钟。
    //rcu_periph_clock_enable(ADC_GPIO_PORT_RCU);
    
	// 启动ADC的时钟。
    //rcu_periph_clock_enable(RCU_ADC);
    
	// 配置ADC的时钟。
    //rcu_adc_clock_config(RCU_CKADC_CKAPB2_DIV4);
	
	 // 将GPIO设置为模拟输入模式。
    //gpio_init(ADC_GPIO_PORT, GPIO_MODE_AIN, GPIO_OSPEED_MAX, ADC_GPIO_PIN);
	
    // 设置ADC的连续转换模式和扫描模式。
    adc_special_function_config(ADC, ADC_CONTINUOUS_MODE, ENABLE);
    adc_special_function_config(ADC, ADC_SCAN_MODE, DISABLE);  
    
	// 配置ADC的触发源。
    adc_external_trigger_source_config(ADC, ADC_REGULAR_CHANNEL, ADC0_1_2_EXTTRIG_REGULAR_NONE); 
    
	// 配置ADC的数据对齐方式。
    adc_data_alignment_config(ADC, ADC_DATAALIGN_RIGHT);
    
	// 配置ADC的工作模式。
    adc_mode_config(ADC_MODE_FREE); 
    
	// 配置ADC的通道长度。
    adc_channel_length_config(ADC, ADC_REGULAR_CHANNEL, 1);
 
    // 配置ADC的常规通道。采样时间也会影响得到的ADC数据，不要搞错
    adc_regular_channel_config(ADC, 0, BOARD_ADC_CHANNEL, ADC_SAMPLETIME_55POINT5);
    adc_external_trigger_config(ADC, ADC_REGULAR_CHANNEL, ENABLE);
  
    // 设置ADC的过采样模式和比例。
    adc_oversample_mode_config(ADC, ADC_OVERSAMPLING_ALL_CONVERT, ADC_OVERSAMPLING_SHIFT_4B, ADC_OVERSAMPLING_RATIO_MUL16);
    adc_oversample_mode_enable(ADC);
  
    // 启动ADC。
    adc_enable(ADC);
    delay_ms(1);
    
	// 启动ADC的校准。
    adc_calibration_enable(ADC);
}
*/

//int main(void)
//{
//		uint16_t adc_value = 0;
//    // 配置系统滴答定时器,用来生成固定的时间间隔执行延时delay函数
//    systick_config();  //使用systick。c时才配置
//    // 配置ADC。
//    adc_config();
//  
//    // 启动ADC的软件触发。
//    adc_software_trigger_enable(ADC, ADC_REGULAR_CHANNEL);
//  
//    // 主循环。
//    while(1){
//        // 清除ADC的结束转换（End of Conversion）标志。
//        adc_flag_clear(ADC, ADC_FLAG_EOC);
//        // 等待ADC的转换完成。
//        while(SET != adc_flag_get(ADC, ADC_FLAG_EOC)){
//        }
//        
//        // 读取ADC的值。
//        adc_value = ADC_RDATA(ADC);
//        // 打印ADC的值。
//        printf("16 times sample, 4 bits shift: 0x%x\r\n", adc_value);
//        // 延迟500毫秒。
//        delay_ms(500);        
//    }
//}



