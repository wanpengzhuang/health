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

/************************�궨��**********************************/
#define BOARD_ADC_CHANNEL   ADC_CHANNEL_3	//ADCͨ��
#define ADC_GPIO_PORT_RCU   RCU_GPIOA		//ADC�˿�ʱ��
#define RCU_ADC				RCU_ADC0		//ADCʱ��
#define ADC					ADC0			//ADC��
#define ADC_IRQn			ADC0_1_IRQn		//�ж������
//#define ADC0_1_EXTTRIG_REGULAR_T1_CH1  ADC0_1_EXTTRIG_REGULAR_T1_CH1//�ⲿ����Դ



/************************�����ⲿ����*****************************/
// ȫ�ֱ������ڴ洢ADCֵ
extern volatile uint16_t g_adc_data;

// �õ���ADC���ݵı�־λ
extern volatile bool flag_get_new_adc_data;
extern volatile int heart_time ;
extern volatile int breath_time ;

void rcu_config(void);
void adc_gpio_config(void);
void adc_config(void);

//PPG**********
void ppg_adc_config(void);

#endif
