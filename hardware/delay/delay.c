/*!
  \file	 	delay.c
  \brief	这个文件包含了两种延时方法：定时器延时和忙等待延时。
			用户可以通过修改宏USE_TIMER_DELAY的值来选择使用哪种延时方法。
			有操作系统时直接用RTOS的延时更好
  \author	chuang
  
  \version	2024-05-16,for GD32F303CC 
  \modify	2024-05-16
			**增加忙等待延时功能:
			  使用SysTick定时器进行忙等待。
			  此时处理器会一直等待，直到指定的时间过去。
			  这种方法比较简单，但在等待期间处理器不能做其他事情。
			  
			**增加定时器延时todo
			  使用TIMER2产生中断来实现精确的毫秒级别延时。
		      在延时期间，处理器可以执行其他任务或者进入低功耗模式。			 
*/


/************************头文件**********************************/
#include "delay.h"



/************************宏定义**********************************/
#define MCU_CLOCK_FREQ_MHZ 		120	//定义处理器频率（单位：MHz）
#define TIMER_FREQUENCY_KHZ     1000//定时器频率
#define USE_TIMER_DELAY 		1 	//定时器延时设为1，忙等待延时设为0


/************************变量定义**********************************/
//volatile uint32_t delay_counter = 0;



/************************函数定义**********************************/
//忙等待延时
void delay_ms(uint32_t n)
{
	while(n--)
	{
		SysTick->CTRL = 0; // 关闭系统定时器
		SysTick->LOAD = MCU_CLOCK_FREQ_MHZ * 1000 - 1; // 1ms = PROCESSOR_FREQUENCY_MHZ * 1000 - 1
		SysTick->VAL = 0; // 清空当前计数值寄存器且会清零COUNTFLAG标志位
		SysTick->CTRL = 5; // 使能系统定时器，它的时钟源为参考时钟=内核时钟（PROCESSOR_FREQUENCY_MHZ MHz）
		while ((SysTick->CTRL & 0x10000) == 0); // Wait until count flag is set
	}
	SysTick->CTRL = 0; // 关闭系统定时器
}

void delay_us(uint32_t n)
{
	SysTick->CTRL = 0; // 关闭系统定时器
	SysTick->LOAD = MCU_CLOCK_FREQ_MHZ * n - 1; // 1us = PROCESSOR_FREQUENCY_MHZ - 1
	SysTick->VAL = 0; // 清空当前计数值寄存器且会清零COUNTFLAG标志位
	SysTick->CTRL = 5; // 使能系统定时器，它的时钟源为参考时钟=内核时钟（PROCESSOR_FREQUENCY_MHZ MHz）
	while ((SysTick->CTRL & 0x10000) == 0); // Wait until count flag is set
	SysTick->CTRL = 0; // 关闭系统定时器
}
