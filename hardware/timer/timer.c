/*!
  \file	 	timer.c
  \brief	定时器配置
  \author	chuang
  
  \version	2024-05-16,for GD32F303CC 
  \modify	2024-05-16
			**增加定时器配置例程		
			**结构体变量区别
		      timer_initpara 主要用于配置定时器的基本计数参数
		      如预分频值、计数周期，计数模式，时钟分频因子，重复计数值
		      timer_ocintpara 主要用于配置定时器通道的输出参数
		      如输出极性，输出状态，占空比，脉冲宽度，输出模式。
			**增加定时器中断控制ADC采样例程
*/

#include "timer.h"

//定时0.1s = 1199U,9999U
#define TIMER_PRESCALER_VALUE   1199U  			// TIMER预分频值
#define TIMER_PERIOD_VALUE      7999U     		// TIMER计数周期值
#define RCU_TIMER				RCU_TIMER1		//定时器时钟号
#define TIMER					TIMER1			//定时器号
#define TIMER_CH				TIMER_CH_0		//定时器通道
#define TIMER_IRQn				TIMER1_IRQn		//定时器中断请求号

//定时2ms
#define TIMER3_PRESCALER_VALUE   11999U  // TIMER3预分频值
#define TIMER3_PERIOD_VALUE      19U     // TIMER3计数周期值

/*
//定时器初始化例程
void timer_config(void)
{
	timer_parameter_struct timer_initpara;  // 定义TIMER初始化结构体

    rcu_periph_clock_enable(RCU_TIMER);  // 使能TIMER时钟

    timer_deinit(TIMER);  // 复位TIMER

    // TIMER3配置 
    timer_initpara.prescaler         = TIMER_PRESCALER_VALUE;  // 预分频值
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;  // 计数方式：边沿对齐
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;  // 计数方向：向上计数
    timer_initpara.period            = TIMER_PERIOD_VALUE;  // 计数周期
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;  // 时钟分频因子
    timer_initpara.repetitioncounter = 0;  // 重复计数值
    timer_init(TIMER, &timer_initpara);  // 初始化TIMER

    // 使能自动重装载预装载 
    timer_auto_reload_shadow_enable(TIMER);
	
    // 使能TIMER更新中断 
    timer_interrupt_enable(TIMER, TIMER_INT_UP);
	
    // 使能TIMER计数器 
    timer_enable(TIMER);
	
	// 配置NVIC
    nvic_priority_group_set(NVIC_PRIGROUP_PRE1_SUB3);  // 设置NVIC优先级分组
    nvic_irq_enable(TIMER_IRQn, 0, 1);  // 使能TIMER中断，第一组的第0级中断优先级
}
*/


//定时器中断控制ADC采样
void timer_config(void)
{
    timer_oc_parameter_struct timer_ocintpara;
    timer_parameter_struct timer_initpara;
	
	// 使能TIMER时钟
	rcu_periph_clock_enable(RCU_TIMER);  
	
	// 复位TIMER
    timer_deinit(TIMER);  

    //定时器配置
    timer_initpara.prescaler         = TIMER_PRESCALER_VALUE;
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.period            = TIMER_PERIOD_VALUE;
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;
    timer_init(TIMER,&timer_initpara);
    
	//初始化配置定时器通道输出参数的结构体实例，用于设置定时器通道的输出特性：PWM
    timer_channel_output_struct_para_init(&timer_ocintpara);

    //通道配置为PWM模式
    timer_ocintpara.ocpolarity  = TIMER_OC_POLARITY_HIGH;
    timer_ocintpara.outputstate = TIMER_CCX_ENABLE;
    timer_channel_output_config(TIMER, TIMER_CH, &timer_ocintpara);

	//PWM1和PWM2输出模式
    timer_channel_output_pulse_value_config(TIMER, TIMER_CH, 3999);
    timer_channel_output_mode_config(TIMER, TIMER_CH, TIMER_OC_MODE_PWM1);
    timer_channel_output_shadow_config(TIMER, TIMER_CH, TIMER_OC_SHADOW_DISABLE);
	
	//使能定时器
	timer_enable(TIMER);
	
	// 配置NVIC
    //nvic_priority_group_set(NVIC_PRIGROUP_PRE1_SUB3);  // 设置NVIC优先级分组
	
	//RTOS中只能用第四组
	nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);
    nvic_irq_enable(TIMER_IRQn, 0, 0);  // 使能TIMER中断，第一组的第0级中断优先级
	
}
void timer3_config(void)
{
	timer_parameter_struct timer_initpara;  // 定义TIMER初始化结构体

    rcu_periph_clock_enable(RCU_TIMER3);  // 使能TIMER3时钟

    timer_deinit(TIMER3);  // 复位TIMER3

    /* TIMER3配置 */
    timer_initpara.prescaler         = TIMER3_PRESCALER_VALUE;  // 预分频值
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;  // 计数方式：边沿对齐
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;  // 计数方向：向上计数
    timer_initpara.period            = TIMER3_PERIOD_VALUE;  // 计数周期
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;  // 时钟分频因子
    timer_initpara.repetitioncounter = 0;  // 重复计数值
    timer_init(TIMER3, &timer_initpara);  // 初始化TIMER3

    /* 使能自动重装载预装载 */
    timer_auto_reload_shadow_enable(TIMER3);
    /* 使能TIMER3更新中断 */
    timer_interrupt_enable(TIMER3, TIMER_INT_UP);
    /* 使能TIMER3计数器 */
    timer_enable(TIMER3);
	
	//RTOS中只能用第四组
	nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);
    nvic_irq_enable(TIMER3_IRQn, 0, 0);  // 使能TIMER中断，第一组的第0级中断优先级
}



