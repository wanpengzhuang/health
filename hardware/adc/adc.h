#ifndef __ADC_H
#define __ADC_H


#include "gd32f30x.h"
#include "systick.h"
#include "delay.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "uart.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/************************宏定义**********************************/
#define BOARD_ADC_CHANNEL   ADC_CHANNEL_3	//ADC通道
#define ADC_GPIO_PORT_RCU   RCU_GPIOA		//ADC端口时钟
#define RCU_ADC				RCU_ADC0		//ADC时钟
#define ADC					ADC0			//ADC号
#define ADC_IRQn			ADC0_1_IRQn		//中断请求号
//#define ADC0_1_EXTTRIG_REGULAR_T1_CH1  ADC0_1_EXTTRIG_REGULAR_T1_CH1//外部触发源



/************************声明外部变量*****************************/
// 全局变量用于存储ADC值
extern volatile uint16_t g_adc_data;

// 得到新ADC数据的标志位
extern volatile bool flag_get_new_adc_data;
extern volatile int heart_time ;
extern volatile int breath_time ;

void rcu_config(void);
void adc_gpio_config(void);
void adc_config(void);

//PPG**********
void ppg_adc_config(void);

#endif
