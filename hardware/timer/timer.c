/*!
  \file	 	timer.c
  \brief	��ʱ������
  \author	chuang
  
  \version	2024-05-16,for GD32F303CC 
  \modify	2024-05-16
			**���Ӷ�ʱ����������		
			**�ṹ���������
		      timer_initpara ��Ҫ�������ö�ʱ���Ļ�����������
		      ��Ԥ��Ƶֵ���������ڣ�����ģʽ��ʱ�ӷ�Ƶ���ӣ��ظ�����ֵ
		      timer_ocintpara ��Ҫ�������ö�ʱ��ͨ�����������
		      ��������ԣ����״̬��ռ�ձȣ������ȣ����ģʽ��
			**���Ӷ�ʱ���жϿ���ADC��������
*/

#include "timer.h"

//��ʱ0.1s = 1199U,9999U
#define TIMER_PRESCALER_VALUE   1199U  			// TIMERԤ��Ƶֵ
#define TIMER_PERIOD_VALUE      7999U     		// TIMER��������ֵ
#define RCU_TIMER				RCU_TIMER1		//��ʱ��ʱ�Ӻ�
#define TIMER					TIMER1			//��ʱ����
#define TIMER_CH				TIMER_CH_0		//��ʱ��ͨ��
#define TIMER_IRQn				TIMER1_IRQn		//��ʱ���ж������

//��ʱ2ms
#define TIMER3_PRESCALER_VALUE   11999U  // TIMER3Ԥ��Ƶֵ
#define TIMER3_PERIOD_VALUE      19U     // TIMER3��������ֵ

/*
//��ʱ����ʼ������
void timer_config(void)
{
	timer_parameter_struct timer_initpara;  // ����TIMER��ʼ���ṹ��

    rcu_periph_clock_enable(RCU_TIMER);  // ʹ��TIMERʱ��

    timer_deinit(TIMER);  // ��λTIMER

    // TIMER3���� 
    timer_initpara.prescaler         = TIMER_PRESCALER_VALUE;  // Ԥ��Ƶֵ
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;  // ������ʽ�����ض���
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;  // �����������ϼ���
    timer_initpara.period            = TIMER_PERIOD_VALUE;  // ��������
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;  // ʱ�ӷ�Ƶ����
    timer_initpara.repetitioncounter = 0;  // �ظ�����ֵ
    timer_init(TIMER, &timer_initpara);  // ��ʼ��TIMER

    // ʹ���Զ���װ��Ԥװ�� 
    timer_auto_reload_shadow_enable(TIMER);
	
    // ʹ��TIMER�����ж� 
    timer_interrupt_enable(TIMER, TIMER_INT_UP);
	
    // ʹ��TIMER������ 
    timer_enable(TIMER);
	
	// ����NVIC
    nvic_priority_group_set(NVIC_PRIGROUP_PRE1_SUB3);  // ����NVIC���ȼ�����
    nvic_irq_enable(TIMER_IRQn, 0, 1);  // ʹ��TIMER�жϣ���һ��ĵ�0���ж����ȼ�
}
*/


//��ʱ���жϿ���ADC����
void timer_config(void)
{
    timer_oc_parameter_struct timer_ocintpara;
    timer_parameter_struct timer_initpara;
	
	// ʹ��TIMERʱ��
	rcu_periph_clock_enable(RCU_TIMER);  
	
	// ��λTIMER
    timer_deinit(TIMER);  

    //��ʱ������
    timer_initpara.prescaler         = TIMER_PRESCALER_VALUE;
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.period            = TIMER_PERIOD_VALUE;
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;
    timer_init(TIMER,&timer_initpara);
    
	//��ʼ�����ö�ʱ��ͨ����������Ľṹ��ʵ�����������ö�ʱ��ͨ����������ԣ�PWM
    timer_channel_output_struct_para_init(&timer_ocintpara);

    //ͨ������ΪPWMģʽ
    timer_ocintpara.ocpolarity  = TIMER_OC_POLARITY_HIGH;
    timer_ocintpara.outputstate = TIMER_CCX_ENABLE;
    timer_channel_output_config(TIMER, TIMER_CH, &timer_ocintpara);

	//PWM1��PWM2���ģʽ
    timer_channel_output_pulse_value_config(TIMER, TIMER_CH, 3999);
    timer_channel_output_mode_config(TIMER, TIMER_CH, TIMER_OC_MODE_PWM1);
    timer_channel_output_shadow_config(TIMER, TIMER_CH, TIMER_OC_SHADOW_DISABLE);
	
	//ʹ�ܶ�ʱ��
	timer_enable(TIMER);
	
	// ����NVIC
    //nvic_priority_group_set(NVIC_PRIGROUP_PRE1_SUB3);  // ����NVIC���ȼ�����
	
	//RTOS��ֻ���õ�����
	nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);
    nvic_irq_enable(TIMER_IRQn, 0, 0);  // ʹ��TIMER�жϣ���һ��ĵ�0���ж����ȼ�
	
}
void timer3_config(void)
{
	timer_parameter_struct timer_initpara;  // ����TIMER��ʼ���ṹ��

    rcu_periph_clock_enable(RCU_TIMER3);  // ʹ��TIMER3ʱ��

    timer_deinit(TIMER3);  // ��λTIMER3

    /* TIMER3���� */
    timer_initpara.prescaler         = TIMER3_PRESCALER_VALUE;  // Ԥ��Ƶֵ
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;  // ������ʽ�����ض���
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;  // �����������ϼ���
    timer_initpara.period            = TIMER3_PERIOD_VALUE;  // ��������
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;  // ʱ�ӷ�Ƶ����
    timer_initpara.repetitioncounter = 0;  // �ظ�����ֵ
    timer_init(TIMER3, &timer_initpara);  // ��ʼ��TIMER3

    /* ʹ���Զ���װ��Ԥװ�� */
    timer_auto_reload_shadow_enable(TIMER3);
    /* ʹ��TIMER3�����ж� */
    timer_interrupt_enable(TIMER3, TIMER_INT_UP);
    /* ʹ��TIMER3������ */
    timer_enable(TIMER3);
	
	//RTOS��ֻ���õ�����
	nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);
    nvic_irq_enable(TIMER3_IRQn, 0, 0);  // ʹ��TIMER�жϣ���һ��ĵ�0���ж����ȼ�
}



