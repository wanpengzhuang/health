/*!
  \file	 	delay.c
  \brief	����ļ�������������ʱ��������ʱ����ʱ��æ�ȴ���ʱ��
			�û�����ͨ���޸ĺ�USE_TIMER_DELAY��ֵ��ѡ��ʹ��������ʱ������
			�в���ϵͳʱֱ����RTOS����ʱ����
  \author	chuang
  
  \version	2024-05-16,for GD32F303CC 
  \modify	2024-05-16
			**����æ�ȴ���ʱ����:
			  ʹ��SysTick��ʱ������æ�ȴ���
			  ��ʱ��������һֱ�ȴ���ֱ��ָ����ʱ���ȥ��
			  ���ַ����Ƚϼ򵥣����ڵȴ��ڼ䴦�����������������顣
			  
			**���Ӷ�ʱ����ʱtodo
			  ʹ��TIMER2�����ж���ʵ�־�ȷ�ĺ��뼶����ʱ��
		      ����ʱ�ڼ䣬����������ִ������������߽���͹���ģʽ��			 
*/


/************************ͷ�ļ�**********************************/
#include "delay.h"



/************************�궨��**********************************/
#define MCU_CLOCK_FREQ_MHZ 		120	//���崦����Ƶ�ʣ���λ��MHz��
#define TIMER_FREQUENCY_KHZ     1000//��ʱ��Ƶ��
#define USE_TIMER_DELAY 		1 	//��ʱ����ʱ��Ϊ1��æ�ȴ���ʱ��Ϊ0


/************************��������**********************************/
//volatile uint32_t delay_counter = 0;



/************************��������**********************************/
//æ�ȴ���ʱ
void delay_ms(uint32_t n)
{
	while(n--)
	{
		SysTick->CTRL = 0; // �ر�ϵͳ��ʱ��
		SysTick->LOAD = MCU_CLOCK_FREQ_MHZ * 1000 - 1; // 1ms = PROCESSOR_FREQUENCY_MHZ * 1000 - 1
		SysTick->VAL = 0; // ��յ�ǰ����ֵ�Ĵ����һ�����COUNTFLAG��־λ
		SysTick->CTRL = 5; // ʹ��ϵͳ��ʱ��������ʱ��ԴΪ�ο�ʱ��=�ں�ʱ�ӣ�PROCESSOR_FREQUENCY_MHZ MHz��
		while ((SysTick->CTRL & 0x10000) == 0); // Wait until count flag is set
	}
	SysTick->CTRL = 0; // �ر�ϵͳ��ʱ��
}

void delay_us(uint32_t n)
{
	SysTick->CTRL = 0; // �ر�ϵͳ��ʱ��
	SysTick->LOAD = MCU_CLOCK_FREQ_MHZ * n - 1; // 1us = PROCESSOR_FREQUENCY_MHZ - 1
	SysTick->VAL = 0; // ��յ�ǰ����ֵ�Ĵ����һ�����COUNTFLAG��־λ
	SysTick->CTRL = 5; // ʹ��ϵͳ��ʱ��������ʱ��ԴΪ�ο�ʱ��=�ں�ʱ�ӣ�PROCESSOR_FREQUENCY_MHZ MHz��
	while ((SysTick->CTRL & 0x10000) == 0); // Wait until count flag is set
	SysTick->CTRL = 0; // �ر�ϵͳ��ʱ��
}
