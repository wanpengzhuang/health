/*!
  \file	 	adc.c
  \brief	ADC����
			**���������CPU���ƴ�������while��1���п��ƣ������ӳ�����ѭ����ʱ
			**��ʱ��Ӳ��������ADCӲ�����������ƣ�ʵʱ�Ըߣ��жϷ������н���
			**DMAģʽ
  \author	chuang
  
  \version	2024-05-16,for GD32F303CC 
  \modify	2024-05-16
			**����Ӳ����ʱ�������汾
			���ú궨�壬��ʱ��1ͨ��3��ADC0ͨ��3������������ԣ���ʱ����������
		    ���Բ�ʹ�ø������ϵĶ�ʱ����ʹ�ö�ʱ��1��ͨ��0������ʾ��ʹ�ò���ͨ�����ɹ�
		    ��ʱ������Ҫ��������ʱ0.02���ֹͣ
			  
			**����DMAģʽtodo			 
*/

/************************ͷ�ļ�**********************************/
#include "adc.h"


//PPG***********
#define PPG_BOARD_ADC_CHANNEL   ADC_CHANNEL_4//ADC_CHANNEL_13
#define PPG_PPG_ADC_GPIO_PORT_RCU   RCU_GPIOA//RCU_GPIOC
#define PPG_ADC_GPIO_PORT       GPIOA//GPIOC
#define PPG_ADC_GPIO_PIN        GPIO_PIN_4//GPIO_PIN_3


/************************��������**********************************/
void rcu_config(void)
{
    //����GPIOʱ��
    rcu_periph_clock_enable(RCU_GPIOA);
	
    //����ADCʱ��,ѹ��
    rcu_periph_clock_enable(RCU_ADC0);
	
    //����ADCʱ�ӣ�ADCʱ��Ƶ��ѡ��CK_APB2ʱ��Ƶ��ΪԭƵ�ʵ�1/4
    rcu_adc_clock_config(RCU_CKADC_CKAPB2_DIV4);
	
}

void adc_gpio_config(void)
{
    //���ö˿�ģʽΪģ�����룬�˿��ٶ�Ϊ����ٶ�
    gpio_init(GPIOA, GPIO_MODE_AIN, GPIO_OSPEED_MAX, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);
	
}

void adc_config(void)
{
	//ADCģʽ���ã�����ģʽ���������ɨ�������ģʽ�¹���
    adc_mode_config(ADC_MODE_FREE);
	
	// ����ADC������ת��ģʽ���Զ������ڵ�ǰͨ��ת������ɨ��ģʽ���Զ��л�ͨ��ת������
    adc_special_function_config(ADC0, ADC_CONTINUOUS_MODE, DISABLE);
    adc_special_function_config(ADC0, ADC_SCAN_MODE, ENABLE); 
	
	//����ADC��������12λ
	//adc_resolution_config(ADC0, ADC_RESOLUTION_12B);
	
	//ADC���ݶ�������
    adc_data_alignment_config(ADC0, ADC_DATAALIGN_RIGHT);
	
    //ADCͨ���������ã�ͬʱ�����ͨ��������ͨ������ͬʱ���ഫ��������
    //adc_channel_length_config(ADC0, ADC_REGULAR_CHANNEL, 1);
	adc_channel_length_config(ADC0, ADC_INSERTED_CHANNEL, 4);
	
	//����ADC�Ĳ���ͨ������ͨ����ȡͬһ�������Ч��
	adc_inserted_channel_config(ADC0, 0, ADC_CHANNEL_0, ADC_SAMPLETIME_55POINT5);
    adc_inserted_channel_config(ADC0, 1, ADC_CHANNEL_1, ADC_SAMPLETIME_55POINT5);
	adc_inserted_channel_config(ADC0, 2, ADC_CHANNEL_2, ADC_SAMPLETIME_55POINT5);
	adc_inserted_channel_config(ADC0, 3, ADC_CHANNEL_3, ADC_SAMPLETIME_55POINT5);
    
	// ����ADC�ĳ���ͨ��������ʱ��Ҳ��Ӱ��õ���ADC���ݣ���Ҫ���
    //adc_regular_channel_config(ADC0, 0, BOARD_ADC_CHANNEL, ADC_SAMPLETIME_55POINT5);
	
	//����ʱ��55.5΢��
    //ADC�������ã���ʱ��1�ĵ�3ͨ��������ADC0_1_2_EXTTRIG_REGULAR_NONE��ʾ�������
    //adc_external_trigger_source_config(ADC0, ADC_REGULAR_CHANNEL, ADC0_1_EXTTRIG_REGULAR_T1_CH3); 
	//adc_external_trigger_source_config(ADC0, ADC_REGULAR_CHANNEL, ADC0_1_2_EXTTRIG_REGULAR_NONE); 
	//ADC�������ã���ʱ��1�ĵ�0ͨ������,timer.c��������ADӲ��������ʱ���������ǵ�ǰ�����ϵĶ�ʱ��
	adc_external_trigger_source_config(ADC0, ADC_INSERTED_CHANNEL, ADC0_1_EXTTRIG_INSERTED_T1_CH0);
	
	//ADC�ⲿ��������
    //adc_external_trigger_config(ADC0, ADC_REGULAR_CHANNEL, ENABLE);
	adc_external_trigger_config(ADC0, ADC_INSERTED_CHANNEL, ENABLE);
	
	// ����ADC�Ĺ�����ģʽ�ͱ�����
    adc_oversample_mode_config(ADC0, ADC_OVERSAMPLING_ALL_CONVERT, ADC_OVERSAMPLING_SHIFT_4B, ADC_OVERSAMPLING_RATIO_MUL16);
    adc_oversample_mode_enable(ADC0);
	
	//���ADC��EOC����ת����־��EOIC���������־,��ʹ�ò���ͨ��ʱ����Ҫ�������
    adc_interrupt_flag_clear(ADC0, ADC_INT_FLAG_EOC);
    adc_interrupt_flag_clear(ADC0, ADC_INT_FLAG_EOIC);
    adc_interrupt_enable(ADC0, ADC_INT_EOIC);//����EOIC�ж�
	
    //����ADC�ӿ�
    adc_enable(ADC0);
    delay_1ms(1);
	//vTaskDelay(1);
	
    //ADCУ׼�͸�λУ׼
    adc_calibration_enable(ADC0);
	
	//����NVIC
	//nvic_priority_group_set(NVIC_PRIGROUP_PRE1_SUB3);
	
	//RTOS��ֻ���õ�����
	nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);
	
	//ʹ��ADC0��ADC1��������ʹ��ADC0_1_IRQn�ж������
	nvic_irq_enable(ADC0_1_IRQn, 0, 0);
	
}

// �жϷ�����򣬲���Ҫ��������ֱ�Ӹ���
void ADC0_1_IRQHandler(void)
{
	
    //static uint16_t adcValue; // ��̬�����洢ADC����
	
	// �ȴ�ADC��ת����ɣ�ע�͵����ж�������ʱ��Ҫ��
	 //while(SET != adc_flag_get(ADC, ADC_FLAG_EOC));
	if(SET == adc_flag_get(ADC, ADC_FLAG_EOC))
	{
		// ��ȡADC����
		//adcValue = adc_regular_data_read(ADC0);//����ͨ��
		g_adc_data = adc_inserted_data_read(ADC0 , ADC_INSERTED_CHANNEL_1);//����ͨ��

		// ��ӡADC����
		//printf("pressure=%d\n", g_adc_data);

		// ����жϱ�־���Ա���һ���ж�ʱ�����ٴδ��������ڲ���������������Ϊ���˲���ͨ��
		//adc_flag_clear(ADC0, ADC_FLAG_EOC);
		
		//���ADC��EOC����ת����־��EOIC���������־,��ʹ�ò���ͨ��ʱ����Ҫ�������
		adc_interrupt_flag_clear(ADC0, ADC_INT_FLAG_EOC);
		adc_interrupt_flag_clear(ADC0, ADC_INT_FLAG_EOIC);
		
		//����ʱ��
		heart_time += 80;
		breath_time +=80;
		
		flag_get_new_adc_data = 1;
	}
}

//PPG**********************
void ppg_adc_config(void)
{
	// ����GPIOA��ʱ�ӡ�
    rcu_periph_clock_enable(PPG_PPG_ADC_GPIO_PORT_RCU);
    // ����ADC1��ʱ�ӡ�
    rcu_periph_clock_enable(RCU_ADC1);
    // ����ADC��ʱ�ӡ�
    rcu_adc_clock_config(RCU_CKADC_CKAPB2_DIV4);
	
	 // ��GPIO����Ϊģ������ģʽ��
    gpio_init(PPG_ADC_GPIO_PORT, GPIO_MODE_AIN, GPIO_OSPEED_MAX, PPG_ADC_GPIO_PIN);
	
    // ����ADC������ת��ģʽ��ɨ��ģʽ��
    adc_special_function_config(ADC1, ADC_CONTINUOUS_MODE, ENABLE);
    adc_special_function_config(ADC1, ADC_SCAN_MODE, DISABLE);  
    // ����ADC�Ĵ���Դ��
    adc_external_trigger_source_config(ADC1, ADC_REGULAR_CHANNEL, ADC0_1_2_EXTTRIG_REGULAR_NONE); 
    // ����ADC�����ݶ��뷽ʽ��
    adc_data_alignment_config(ADC1, ADC_DATAALIGN_RIGHT);
    // ����ADC�Ĺ���ģʽ��
    adc_mode_config(ADC_MODE_FREE); 
    // ����ADC��ͨ�����ȡ�
    adc_channel_length_config(ADC1, ADC_REGULAR_CHANNEL, 1);
 
    // ����ADC�ĳ���ͨ��������ʱ��Ҳ��Ӱ��õ���ADC���ݣ���Ҫ���
    adc_regular_channel_config(ADC1, 0, PPG_BOARD_ADC_CHANNEL, ADC_SAMPLETIME_239POINT5);
    adc_external_trigger_config(ADC1, ADC_REGULAR_CHANNEL, ENABLE);
  
    // ����ADC�Ĺ�����ģʽ�ͱ�����
    adc_oversample_mode_config(ADC1, ADC_OVERSAMPLING_ALL_CONVERT, ADC_OVERSAMPLING_SHIFT_4B, ADC_OVERSAMPLING_RATIO_MUL16);
    adc_oversample_mode_enable(ADC1);
  
    // ����ADC��
    adc_enable(ADC1);
    delay_ms(1);
    // ����ADC��У׼��
    adc_calibration_enable(ADC1);
	
	//RTOS��ֻ���õ�����
	nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);
	
	//ʹ��ADC0��ADC1��������ʹ��ADC0_1_IRQn�ж������
	nvic_irq_enable(ADC0_1_IRQn, 0, 0);
}


/*
//���������������
void adc_config(void)
{
	// ����GPIOA��ʱ�ӡ�
    //rcu_periph_clock_enable(ADC_GPIO_PORT_RCU);
    
	// ����ADC��ʱ�ӡ�
    //rcu_periph_clock_enable(RCU_ADC);
    
	// ����ADC��ʱ�ӡ�
    //rcu_adc_clock_config(RCU_CKADC_CKAPB2_DIV4);
	
	 // ��GPIO����Ϊģ������ģʽ��
    //gpio_init(ADC_GPIO_PORT, GPIO_MODE_AIN, GPIO_OSPEED_MAX, ADC_GPIO_PIN);
	
    // ����ADC������ת��ģʽ��ɨ��ģʽ��
    adc_special_function_config(ADC, ADC_CONTINUOUS_MODE, ENABLE);
    adc_special_function_config(ADC, ADC_SCAN_MODE, DISABLE);  
    
	// ����ADC�Ĵ���Դ��
    adc_external_trigger_source_config(ADC, ADC_REGULAR_CHANNEL, ADC0_1_2_EXTTRIG_REGULAR_NONE); 
    
	// ����ADC�����ݶ��뷽ʽ��
    adc_data_alignment_config(ADC, ADC_DATAALIGN_RIGHT);
    
	// ����ADC�Ĺ���ģʽ��
    adc_mode_config(ADC_MODE_FREE); 
    
	// ����ADC��ͨ�����ȡ�
    adc_channel_length_config(ADC, ADC_REGULAR_CHANNEL, 1);
 
    // ����ADC�ĳ���ͨ��������ʱ��Ҳ��Ӱ��õ���ADC���ݣ���Ҫ���
    adc_regular_channel_config(ADC, 0, BOARD_ADC_CHANNEL, ADC_SAMPLETIME_55POINT5);
    adc_external_trigger_config(ADC, ADC_REGULAR_CHANNEL, ENABLE);
  
    // ����ADC�Ĺ�����ģʽ�ͱ�����
    adc_oversample_mode_config(ADC, ADC_OVERSAMPLING_ALL_CONVERT, ADC_OVERSAMPLING_SHIFT_4B, ADC_OVERSAMPLING_RATIO_MUL16);
    adc_oversample_mode_enable(ADC);
  
    // ����ADC��
    adc_enable(ADC);
    delay_ms(1);
    
	// ����ADC��У׼��
    adc_calibration_enable(ADC);
}
*/

//int main(void)
//{
//		uint16_t adc_value = 0;
//    // ����ϵͳ�δ�ʱ��,�������ɹ̶���ʱ����ִ����ʱdelay����
//    systick_config();  //ʹ��systick��cʱ������
//    // ����ADC��
//    adc_config();
//  
//    // ����ADC�����������
//    adc_software_trigger_enable(ADC, ADC_REGULAR_CHANNEL);
//  
//    // ��ѭ����
//    while(1){
//        // ���ADC�Ľ���ת����End of Conversion����־��
//        adc_flag_clear(ADC, ADC_FLAG_EOC);
//        // �ȴ�ADC��ת����ɡ�
//        while(SET != adc_flag_get(ADC, ADC_FLAG_EOC)){
//        }
//        
//        // ��ȡADC��ֵ��
//        adc_value = ADC_RDATA(ADC);
//        // ��ӡADC��ֵ��
//        printf("16 times sample, 4 bits shift: 0x%x\r\n", adc_value);
//        // �ӳ�500���롣
//        delay_ms(500);        
//    }
//}



