/*!
  \file	 	main.c
  \brief	�������
  \author	chuang
  \version	2024-05-16,for GD32F303CC  
*/

/************************ͷ�ļ�**********************************/
#include "gd32f30x.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "systick.h"
#include "uart.h"
#include "delay.h"
#include "adc.h"
#include "timer.h"
#include "gpio.h"

//����
#include "gd_i2c.h"
#include "mlx90614.h"

//RTOS
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
 
#include <math.h>



/************************�궨��**********************************/

#define SAMPLING_RATE_HZ 		13		//ϵͳ������Ϊ13Hz
#define CUTOFF_FREQUENCY_HZ 	5		//���˵õ�5hz�����ź�
#define WINDOW_SIZE 			20		//�ƶ�ƽ���˲���
#define HEART_RATE_THRESHOLD 	25 		//����������ֵ
#define MAX_HEART_NUM 			5		//���������������
#define MAX_BREATH_NUM 			5 		//����������������
#define MAX(a, b) ((a) > (b)? (a) : (b))
#define MIN(a, b) ((a) < (b)? (a) : (b))

//PPG
#define  IBI_array_size 10 	//RR��������10
#define  HRV_need_size  50 //����HRV��Ҫ����������300
#define MAX_BPM_ARRAY_NUM   5   //���������������
#define PPG_HEART_RATE_THRESHOLD 	300  //����������ֵ

/****************************�ṹ�嶨��*************************************/

// ���忨�����˲����ṹ��
typedef struct 
{
    float q; // ������������,Խ��Խ������ģ��Ԥ��ֵ
    float r; // �����������Խ��Խ�������²���ֵ�ĳ̶�
    float x; // ����ֵ
    float p; // ����ֵ���Խ��Խ�����ŵ�ǰԤ��״̬
    float k; // ����������
} KalmanFilter;


// ����ṹ���������ͨ�˲���״̬
typedef struct 
{
    float lastInput; // ��һ������ֵ
    float lastOutput; // ��һ�����ֵ
} HighPassFilter;



/************************ȫ�ֱ�������*******************************/

//�½�������Ҫ�ȶ���������
TaskHandle_t start_task_Handler;
TaskHandle_t task1_Handler;
TaskHandle_t task2_Handler;
TaskHandle_t task3_Handler;

// ȫ�ֱ������ڴ洢ADCֵ
volatile uint16_t g_adc_data = 0;

// ��ʼ�����˺��ֵ
float filtered_value = 0; 
float pre_filtered_value = 0; 

// �õ���ADC���ݵı�־λ
volatile bool flag_get_new_adc_data = false;

// ����alpha (��ͨ�˲���ϵ��)
const float RC = 1.0f / (2 * 3.14159f * CUTOFF_FREQUENCY_HZ);
const float DT = 1.0f / SAMPLING_RATE_HZ;
const float ALPHA = DT / (RC + DT);

KalmanFilter kf;//���忨�����˲���

volatile bool rising_flag = false; // ������־
volatile bool falling_flag = false; // �½���־
volatile float temp_valley = 0;//����
volatile float temp_peak = 0;//����
float heart_peak = 0;//��������
float temp_fuck = 0;
volatile int heart_time = 0;
int heart_rate = 0;
int avg_heart_rate = 0;
int heart_rate_sum = 0;
int get_herat_rate_num = 0;
int heart_rate_array[MAX_HEART_NUM];
int index = 0;//������������

//�������ھֲ�����ֵ������
volatile int breath_time = 0;
int breath_rate = 0;
float breath_peak = 0;
float breath_valley = 0;

//�������
volatile int breath_rates[MAX_BREATH_NUM]; // �洢�����κ�����
int breath_index = 0; // �������������κ���������ı���

bool should_replace_min = true; // ���ڽ������´��滻�������Сֵ

volatile int bodymove_delay = 0;//��ʱ��ʾ�嶯


//���¼��**************************
float temp;//ȫ�ֱ����¶�

//PPG
volatile int BPM;				//����

volatile int Signal;		//�ɼ�����ԭʼ�ź�ֵuint16_t �������������1023����
volatile int pre_signal;		//�ɼ�����ԭʼ�ź�ֵ
volatile  int IBI = 600;	//�������ʱ��

int IBI_array[IBI_array_size] = {0}; //����IBIֵ
int copy_IBI_data[HRV_need_size] = {0};
volatile int IBI_count = 0;
volatile int HRV_need_count = 0;//����HRV�����ʼ�����������
unsigned long sampleCounter = 0;	//ȷ������ʱ��
volatile uint16_t runningTotal = 0;

int i;
int for_loop_num = 0;
float SDNN = 0;
float RMSSD = 0;
float pre_SDNN = 0;
float pre_RMSSD = 0;
volatile int NN50 = 0;
float PNN50 =0;
int NN_total = 0;

int avg_bpm = 0;
int bpm_sum = 0;
int bpm_array[MAX_BPM_ARRAY_NUM] = {0};


float ppg_valley = 0;
float ppg_peak = 0;
volatile bool ppg_rising_flag = false; // ������־
volatile bool ppg_falling_flag = false; // �½���־

int temp_IBI_array_change = 0;


/************************��������**********************************/

//��������
void start_task(void *pvParameters);

void task1(void *pvParameters);

void task2(void *pvParameters);

void task3(void *pvParameters);

void start_os(void);

//��ͨ�˲�
float low_pass_filter(float current_sample, float previous_filtered);

// ��ʼ���������˲���
void kalman_init(KalmanFilter* filter, float q, float r, float init_x, float init_p) ;

// ���¿������˲���״̬
float kalman_update(KalmanFilter* filter, float measurement) ;

//��ʼ����ͨ�˲���
void initFilter(HighPassFilter* filter) ;

//Ӧ�ø�ͨ�˲���
float applyFilter(HighPassFilter* filter, float input, float dt, float cutoffFreq) ;

//����������,�Ƚϲ��岨�ȣ����ȵ�����ķ��ȴ���60�ж�Ϊ�������µĲ���
int find_peak(float adc_value, float pre_adc_value);

//���º���
float update_breath(float new_sample) ;

//�ƶ�ƽ���˲���
uint16_t moving_average_filter(uint16_t new_sample) ;

//���渲�����������е����ֵ��Сֵ
void insert_alternate_min_max(int new_heart_rate);

//PPG**********************
void calculate_heart(void);

// �����׼��SDNN
float calculate_SD(int data[], int n) ;

// ʵ��RMSSD����
float calculate_RMSSD(int data[], int n);

// ʵ�ּ��� NN50 �ĺ���
int calculate_nn50(int data[], int n);

// ʵ�� PANN_5 ����
float calculate_pnn50(int nn50_count, int total_beats) ;

// ����HRV
float calculate_hrv(int rate[], int size) ;



/**************************������**********************************/
int main(void)
{

/**************************��������**********************************/
	
	
	
	
/***********************������ʼ��**********************************/	
	
	// ��ʼ�����ڴ�ӡ
	USART_Init();
	
	//��ʱ������ʼ��
	//delay_init() ;
	
	//����ϵͳ�δ�ʱ��,������ʱdelay����
    systick_config();
	
	// ��ʼ������ʱ��
	rcu_config();
	
	//��ʼ������ADC_GPIO
	adc_gpio_config();
	
	//��ʼ�����ö�ʱ��
	timer_config();
	
	//��ʼ������ADC
    adc_config();
	
	//PPG******************
	ppg_adc_config();
	//����ADC�����������
    adc_software_trigger_enable(ADC1, ADC_REGULAR_CHANNEL);
	//��ʼ����ʱ��
	timer3_config();
	//PPG*****************
	
	//��ʼ���������˲���
	kalman_init(&kf, 0.3f, 0.1f, 0.0f, 1.0f);
	
	//����ϵͳ�ж����ȼ�����ֻ��ѡ���4�飨ֻ����ռ���ȼ���û����Ӧ���ȼ���
	//��ʼ��freertos��ϵͳ��ʱ���ж�Ƶ��ΪconfigTICK_RATE_HZ 120000000/1000
    systick_config();
	
	//����freertos
	start_os();


	
/*************************��ѭ��**********************************/	
	while(1);
	
}


/************************����������*******************************/

void start_os(void)
{
	xTaskCreate(start_task,"start_task",128,NULL,1,&start_task_Handler);
	vTaskStartScheduler();
}

void start_task(void *pvParameters)
{
	taskENTER_CRITICAL();
	
	//��������Խ��Խ��
	xTaskCreate(task1,"task1",1024,NULL,3,&task1_Handler);
	xTaskCreate(task2,"task2",128,NULL,2,&task2_Handler);
	xTaskCreate(task3,"task3",128,NULL,1,&task3_Handler);
	vTaskDelete(start_task_Handler);
	taskEXIT_CRITICAL();
}

//ÿ�����񶼱������while������vTaskDelay��ȷ�������ִ������
//vTaskDelay����Զ�ʱ�����滹�о��Զ�ʱ����vTaskDelayUntil()
void task1(void *pvParameters)
{
	int for_loop_num = 0;
	
	//�����ͨ�˲���
	HighPassFilter hpFilter;
	
	//��ʼ����ͨ�˲���
	initFilter(&hpFilter);
	
	// ��ʼ����������
    for(for_loop_num = 0; for_loop_num < MAX_HEART_NUM; for_loop_num++) 
	{
        heart_rate_array[for_loop_num] = 0;
    }
	
	while(1)
	{		
		uint16_t adc_data_copy = 0;
		
		//PPG**************
		printf("signal = %u\r\n",Signal);//ԭʼ�ź�
		if(BPM>50 && BPM<100)
		{
			printf("BPM = %d\r\n",BPM);//����
			printf("IBI = %d\r\n",IBI);//�������
//			if(temp_IBI_array_change != IBI_array[IBI_array_size-1])
//			{
//				//HRV��������,������������ЧIBIֵ
//				calculate_hrv(IBI_array, IBI_array_size);		
//				temp_IBI_array_change = IBI_array[IBI_array_size-1];

//			}
		}	
		//PPG*************		
	
		if(flag_get_new_adc_data)
		{
			
			// �����жϻ�ʹ������ͬ�������������ٽ�������ֹ�жϴ��
			//�����ٽ������ٽ�������Ҫ����
			taskENTER_CRITICAL();
			
            // ����ȫ�ֱ����Ա��⾺̬����
            adc_data_copy = g_adc_data;
            
            // ��������ݱ�־
            flag_get_new_adc_data = false;
            
            // ���������жϻ����ͬ������
			//���������жϻ����ͬ�������˳��ٽ���
			taskEXIT_CRITICAL();
			
			//������һ��ֵ
			pre_filtered_value = filtered_value;
			
			//�˲�����
			//��ADCֵת��Ϊ�����Ͳ�Ӧ�õ�ͨ�˲���
			filtered_value = low_pass_filter((float)adc_data_copy, filtered_value);	
			
			//�������
			update_breath(filtered_value);
			//��λ���۲�
			//printf("pressure=%d\n", (int)filtered_value);
			
			//Ӧ�ø�ͨ�˲���������������Ϊ0.08�룬��ֹƵ��Ϊ0.8Hz
			filtered_value = applyFilter(&hpFilter, filtered_value, 0.08f, 0.8f);
			
			//�嶯���
			if(filtered_value > 100)
			{
				bodymove_delay = 30;
			}
			if(bodymove_delay)
			{
				printf("bodymove=1\r\n");
				bodymove_delay--;
			}

			
			//��λ���۲�
			//printf("pressure=%d\n", (int)filtered_value);
			
			
			//*5�Ŵ�
			filtered_value = filtered_value*5;
			
			//�������˲�ƽ������
			//filtered_value = kalman_update(&kf, filtered_value);
				
			//���ʼ��
			heart_rate = find_peak(filtered_value,pre_filtered_value);	
				
			// ��λ���۲�
			printf("pressure=%d\r\n", (int)filtered_value);
			
			if(heart_rate>50 && heart_rate<100)
			{
				//HRV���
//				hrv_sum += abs(heart_rate - pre_heart_rate)
//				hrv_num++;
//				hrv_level = hrv_sum/hrv_num;
				
				//get_herat_rate_num++;
				//heart_rate_sum += heart_rate;
				
				//�����������ݵ�����
				//heart_rate_array[index] = heart_rate;
				//ѭ�����������Сֵ����
				insert_alternate_min_max(heart_rate);
				
				// ʹ��ģ����ȷ�����������鷶Χ��
				index = (index + 1) % MAX_HEART_NUM; 
				
				//����������ƽ��ֵ
				heart_rate_sum = 0;
				for( for_loop_num=0;for_loop_num<MAX_HEART_NUM;for_loop_num++)
				{
					heart_rate_sum += heart_rate_array[for_loop_num];
				}
				
				//����ƽ������
				if(heart_rate_array[MAX_HEART_NUM-1])
				{
					avg_heart_rate = heart_rate_sum/MAX_HEART_NUM;
					
					printf("Heart rate=%d\r\n",avg_heart_rate);
				}
			}
			
		}	

		//��ʱ100ms,��ʱ�ᱻ����������ռ
		vTaskDelay(10);
	}
	
}


void task2(void *pvParameters)
{
	//IIC��ʼ��
	gd_i2c_init();

	//mlx90614��ʼ���������
	mlx90614_Init(); 
	
	while(1)
	{
		//��ʵ�¶ȣ���ͷ2���ʣ���2.5����
		float real_temp = SMBus_ReadTemp();
		
		//С��39ʱ������ǹ����
//		if(real_temp<39 && real_temp>32)
//		{
//			temp = real_temp + ((39.0f-real_temp)/2);
//		}
//		else
		{
			temp = real_temp;
		}
		
		if(temp)
			printf("Current Temp: %.1f\r\n", temp);
		
		//PPG*********************
		//printf("signal = %u\r\n",Signal);//ԭʼ�ź�
		if(BPM>50 && BPM<100)
		{
			//printf("BPM = %d\r\n",BPM);//����
			//printf("IBI = %d\r\n",IBI);//�������
			if(temp_IBI_array_change != IBI_array[IBI_array_size-1])
			{
				//HRV��������,������������ЧIBIֵ
				calculate_hrv(IBI_array, IBI_array_size);		
				temp_IBI_array_change = IBI_array[IBI_array_size-1];

			}
		}
		//PPG***************************
		
		vTaskDelay(500);
	}
	
}

void task3(void *pvParameters)
{
	//const char command[] = "\x8A"; // ��ȷ��ʮ����������
	
	//��ʼ����������
	USART1_init(38400U);
	
	//�����������������ģ��
	send_at_command("\x8A");
	
	while(1)
	{

		vTaskDelay(100);
	}
	
}

/*************************��������*************************************/
// �����׼��SDNN 
float calculate_SD(int data[], int n) 
{
    float sum = 0.0, mean, standardDeviation = 0.0;

    int i;
    for (i = 0; i < n; ++i) 
	{
        sum += data[i];
    }
    mean = sum / n;

    for (i = 0; i < n; ++i) 
	{
        standardDeviation += pow(data[i] - mean, 2);
    }

	// ����ʹ��n-1��Ϊ��ĸ����Ϊ����������׼��
    return sqrt(standardDeviation / (n-1)); 
}

// ʵ��RMSSD����
float calculate_RMSSD(int data[], int n) 
{
    float rmssd = 0.0;
    int i;
    
    for (i = 0; i < n - 1; ++i) { // ע������ѭ����n-2����Ϊ���ǱȽϵ������ڼ��
        rmssd += pow(data[i + 1] - data[i], 2);
    }
    
    return sqrt(rmssd / (n - 1)); // ע������ʹ��n-1��Ϊ��ĸ
}

// ʵ�ּ��� NN50 �ĺ���
int calculate_NN50(int data[], int n) 
{
    //static int NN50 = 0;
    int i;
    
    for (i = 0; i < n - 1; ++i) { // ͬ���أ�ѭ����n-2
        if (abs(data[i + 1] - data[i]) > 50)
            NN50++;
    }
    
    return NN50;
}

// ʵ�� PNN50 ����
float calculate_PNN50(int nn50_count, int total_beats) 
{
   return ((float)nn50_count / total_beats) * 100.0f; // ת���ɰٷֱ���ʽ
}



// ����HRV
float calculate_hrv(int IBI_data[], int size) 
{
    int i = 0;
	//static int NN_total = 0;
	
	//������RR��������
	NN_total += size;
	
	//����NN50
	calculate_NN50(IBI_data, size); 
	PNN50 = calculate_PNN50(NN50, NN_total);
	printf("NN50 = %d\r\n",(int)NN50);//NN50
	printf("PNN50 = %d\r\n",(int)PNN50);//PNN50
	
    for ( i = 0; i < IBI_array_size; ++i) 
	{
		copy_IBI_data[HRV_need_count] = IBI_data[i]; 
		
		//��������
		HRV_need_count++;
		if(HRV_need_count == HRV_need_size)
		{
			HRV_need_count = 0;
		}
    }
	
	//����600���ڹ��˵���һ����������
	if(copy_IBI_data[HRV_need_size-1] && copy_IBI_data[0]>600)
	{
		pre_SDNN = SDNN;
		pre_RMSSD = RMSSD;
		
		// ʹ�ñ�׼��SDNN
		SDNN = calculate_SD(copy_IBI_data, HRV_need_size); 
		
		RMSSD = calculate_RMSSD(copy_IBI_data, HRV_need_size); 	
		
		if(pre_SDNN && pre_RMSSD)
		{
			SDNN = (pre_SDNN+SDNN)/2;
			RMSSD = (pre_RMSSD+RMSSD)/2;
		}
		
		if(SDNN<150)
			printf("SDNN = %d\r\n",(int)SDNN);//SDNN
		if(RMSSD<120)
			printf("RMSSD = %d\r\n",(int)RMSSD);//RMSSD
		
	}

	return 0;
}

//��������
void calculate_heart(void)
{
	//���ADC�Ľ���ת����End of Conversion����־��
	adc_flag_clear(ADC1, ADC_FLAG_EOC);
	
	// �ȴ�ADC��ת����ɡ�
	while(SET != adc_flag_get(ADC1, ADC_FLAG_EOC));

	//������һ��ADCֵ
	pre_signal = Signal;
	
	//��ȡADC��ֵ��ADֵ������λ���������ݱ���һ����
	Signal = ADC_RDATA(ADC1)>>2;
	
	//����ʱ��
	sampleCounter += 2;


	if(pre_signal < Signal)
	{
		//���֮ǰһֱ���½�������ͻȻ������������Ϊ����
		if(ppg_falling_flag)
		{
			//��������������
			ppg_valley = pre_signal;

			//���㲨�嵽���ȣ��½��أ�
			//���Ҳ�õ��˲��壬������½��ط��ȴ���������ֵΪ����
			if(ppg_peak && abs(ppg_peak - ppg_valley)>PPG_HEART_RATE_THRESHOLD)
			{

				//�����������,�����쳣��������
				if((60000 / sampleCounter > 50) && (60000 / sampleCounter <120))
				{
					IBI = sampleCounter;
					//BPM = 60000 / sampleCounter;
				}

				//����ʱ��
				sampleCounter = 0;

				//���ò��Ȳ���
				ppg_valley = 0;
				ppg_peak = 0;
			}
		}

		ppg_rising_flag = true;
        ppg_falling_flag = false;

	}


	if(pre_signal > Signal)
	{
		//���֮ǰһֱ������������ͻȻ�½���������Ϊ����
		if(ppg_rising_flag)
		{
			//��������������
			ppg_peak = pre_signal;

			// //���㲨�ȵ����壨�����أ�
			// //���Ҳ�õ��˲��ȣ�����������ط��ȴ���������ֵΪ����
			// if(ppg_valley && abs(ppg_peak - ppg_valley)>PPG_HEART_RATE_THRESHOLD)
			// {

			// 	//�����������,�����쳣��������
			// 	if((60000 / sampleCounter > 50) && (60000 / sampleCounter <120))
			// 	{
			// 		IBI = sampleCounter;
			// 		BPM = 60000 / sampleCounter;
			// 	}

			// 	//����ʱ��
			// 	sampleCounter = 0;

			// 	//���ò��Ȳ���
			// 	ppg_valley = 0;
			// 	ppg_peak = 0;
			// }
		}

		ppg_rising_flag = false;
        ppg_falling_flag = true;

	}

	//ʹ��sampleCounter�ж��Ƿ�õ���IBI	
	if(!sampleCounter)
	{
		//ѭ�����鱣��IBI
		if(IBI_count >= IBI_array_size)
		{
			IBI_count = 0;
		
			IBI_array[IBI_count] = IBI;

			IBI_count++;
		}
		else
		{
			IBI_array[IBI_count] = IBI;
			
			IBI_count++;
		}

		//��������
		if(IBI_array[IBI_array_size - 1])
		{
			runningTotal = 0;
			for( i = 0;i<IBI_array_size;i++)
			{
				runningTotal += IBI_array[i];
			}
			//IBI��ƽ��
			runningTotal /= IBI_array_size;			
			
			//������
			BPM = 60000/runningTotal ;
		}
	}
	
	//��ʱ����ʱ��
	if(sampleCounter>1200)
	{
		sampleCounter = 0;
	}
}

//��ʱ��3���жϷ�����
//ֻ��ISR�����ñ�Ҫ��״̬���߱�־λ��Ȼ������ѭ�������������ﴦ����Щ״̬��
void TIMER3_IRQHandler(void)
{
    if(timer_interrupt_flag_get(TIMER3, TIMER_INT_UP) != RESET)
    {
		//���ñ�־λ����ѭ����ִ�д����߼�
		 //get_adc_data_flag = 1;
		//ADC�źŴ���
		calculate_heart();
		
		// �����ʱ�������жϱ�־
		timer_interrupt_flag_clear(TIMER3, TIMER_INT_UP);
		
	}
}



//���渲�����������е����ֵ��Сֵ
void insert_alternate_min_max(int new_heart_rate) 
{
    int *target_index_ptr; // ָ��Ҫ�滻������������ָ��
    int max_val = heart_rate_array[0];
    int min_val = heart_rate_array[0];
    int max_index = 0;
    int min_index = 0;
	int i = 0;

    // ���������ҵ����ֵ����Сֵ��������
    for ( i = 0; i < MAX_HEART_NUM; ++i) 
	{
        if (heart_rate_array[i] > max_val) 
		{
            max_val = heart_rate_array[i];
            max_index = i;
        }
        if (heart_rate_array[i] < min_val) 
		{
            min_val = heart_rate_array[i];
            min_index = i;
        }
    }

    // ���ݱ��ѡ���滻���ֵ������Сֵ
    if(should_replace_min || (new_heart_rate > avg_heart_rate) || (new_heart_rate < avg_heart_rate-15)) //
	{
        target_index_ptr = &min_index;
		// �滻Ŀ����������ֵ
		heart_rate_array[*target_index_ptr] = new_heart_rate;
    } 	
//	else if(new_heart_rate < avg_heart_rate-15)
//	{
//		//���������ݣ�����
//	}	 
	else
	{
        target_index_ptr = &max_index;
		// �滻Ŀ����������ֵ
		heart_rate_array[*target_index_ptr] = new_heart_rate;
    }

    // �滻Ŀ����������ֵ
    //heart_rate_array[*target_index_ptr] = new_heart_rate;

	if(heart_rate_array[MAX_HEART_NUM-1])
	{
		// �л�����Ա��´��滻��һ��ֵ
		should_replace_min = !should_replace_min;
	}
}


//�ƶ�ƽ���˲���
uint16_t moving_average_filter(uint16_t new_sample) 
{
	//��̬�ֲ�����ֻ��ʼ��һ��
	static float window_sum = 0;
	static int window_index = 0;
	static float window[WINDOW_SIZE] = {0};
	
    // ���ܺ��м�ȥ��ɵ�����ֵ
    window_sum -= window[window_index];
    
    // ���µ�����ֵ�ӵ��ܺ��У���������ӵ���������������
    window_sum += new_sample;
    window[window_index] = new_sample;
    
    // �����������������������߽磬��ص����鿪ʼ����ʵ��ѭ����
    window_index = (window_index + 1) % WINDOW_SIZE;

    // ����ƽ��ֵ��Ϊ���˽��
    return window_sum / WINDOW_SIZE;
}

//�������
float update_breath(float new_sample) 
{
	float temp_breath_data = 0;
	//float temp_breath_rate = 0;
	static float pre_breath_data = 0;
	volatile static bool start_find_breath_flag = 0;
	int loop_num = 0;
	float average_breath_rate = 0;
	float breath_sum = 0;
	
	//�ƶ�ƽ���˲�
	temp_breath_data = moving_average_filter(new_sample);
	
	//��λ���۲�
	//printf("pressure=%d\n", (int)temp_breath_data);
	
	//�����ж�
	if(!start_find_breath_flag && pre_breath_data < temp_breath_data)
	{
		//��ʼ�����������־λ
		start_find_breath_flag = 1;
		
		//���²���
		breath_valley = pre_breath_data;
		
		//Ϊ��ֹ��������������м䱻��ϣ�ֻȡһ��Ҳ���ǲ��ȵ�����
		//����ʱ��
		breath_time = 0;
	}
	
	//������Ҫ���ݲ���������ֵɸѡ�Ǻ�������
	if(start_find_breath_flag && (pre_breath_data > temp_breath_data) )
	{
		breath_peak = pre_breath_data;
		
		//�ж��ǲ��Ǻ���
		if((pre_breath_data - breath_valley) > 40 && (pre_breath_data - breath_valley) < 300 && breath_time>500)
		{
			//�õ���������
			breath_rate = 60000/(breath_time*2);
			
			// ���µĺ�������ӵ������κ�����������
            breath_rates[breath_index] = breath_rate;
            
            // ��������
            breath_index = (breath_index + 1) % MAX_BREATH_NUM;
			
			// ����������ƽ��������
			for (loop_num = 0; loop_num < MAX_BREATH_NUM; ++loop_num) 
			{
				breath_sum += breath_rates[loop_num];
			}
			
			if(breath_rates[MAX_BREATH_NUM-1])
			{
				//���ƽ��������
				average_breath_rate = breath_sum / MAX_BREATH_NUM;
				if(average_breath_rate<25)
				{
					printf("Breath rate=%d\r\n",(int)average_breath_rate);
			
				}
			}

		}
		
		
		
		//���ÿ�ʼ��������־λ
		start_find_breath_flag = 0;
	}
	
	//������һ���ƶ�ƽ���˲�ֵ
	pre_breath_data = temp_breath_data;
	
	return average_breath_rate;

}

//����������,�Ƚϲ��岨�ȣ����ȵ�����ķ��ȴ���������ֵ�ж�Ϊ�������µĲ���
int find_peak(float adc_value, float pre_adc_value)
{
	int temp_heart_rate = 0;
	
	if(adc_value > pre_adc_value)
    {
		//���֮ǰһֱ���½�������ͻȻ������������Ϊ����
		if(falling_flag)
		{ 
			//temp_valley = adc_value;
			//����Ӧ������һ����
			temp_valley = pre_adc_value;
			
			//���㲨�嵽���ȣ��½��أ�
			//���Ҳ�õ��˲��ȣ�������½��ط��ȴ���������ֵΪ����
			if(temp_peak && abs(temp_peak - temp_valley)>HEART_RATE_THRESHOLD)
			{
				temp_fuck = abs(temp_valley -temp_peak);
				temp_heart_rate = 60000 / heart_time;
				
				//�õ���������
				heart_peak = temp_peak;
				
				//���¼���ʱ��
				heart_time = 0;
			}
		}
		
        rising_flag = true;
        falling_flag = false;
    }
	
    if(adc_value < pre_adc_value)
    {
		//���֮ǰһֱ������������ͻȻ�½���������Ϊ����
		if(rising_flag)
		{ 
			//temp_peak = adc_value;
			//����Ӧ������һ����
			temp_peak = pre_adc_value;
			
//			//���㲨�ȵ����壨�����أ�
//			//���Ҳ�õ��˲��壬����������ط��ȴ���������ֵΪ����
//			if(temp_valley && abs(temp_peak - temp_valley)>HEART_RATE_THRESHOLD)
//			{
//				temp_fuck = abs(temp_valley -temp_peak);
//				temp_heart_rate = 60000 / heart_time;
//				
//				//�õ���������
//				heart_peak = temp_peak;
//				
//				//���¼���ʱ��
//				heart_time = 0;
//			}		
		}
		
        falling_flag = true;
        rising_flag = false;
    }
	
	return temp_heart_rate;

}


//һ��IIR��ͨ�˲���
float low_pass_filter(float current_sample, float previous_filtered) 
{
    return ALPHA * current_sample + (1 - ALPHA) * previous_filtered;
}



// ��ʼ����ͨ�˲���
void initFilter(HighPassFilter* filter) 
{
    filter->lastInput = 0.0f;
    filter->lastOutput = 0.0f;
}



// Ӧ�ø�ͨ�˲���
float applyFilter(HighPassFilter* filter, float input, float dt, float cutoffFreq) 
{
    float RC = 1.0f / (2 * 3.14159265358979323846 * cutoffFreq);
    float alpha = dt / (RC + dt);
    float output = alpha * (filter->lastOutput + input - filter->lastInput);
    
    filter->lastInput = input;
    filter->lastOutput = output;
    
    return output;
}



// ��ʼ���������˲���
void kalman_init(KalmanFilter* filter, float q, float r, float init_x, float init_p) 
{
    filter->q = q;
    filter->r = r;
    filter->x = init_x;
    filter->p = init_p;
}


// ���¿������˲���״̬
float kalman_update(KalmanFilter* filter, float measurement) 
{
    // Ԥ�ⲽ��
    filter->p = filter->p + filter->q;

    // ���²���
    filter->k = filter->p / (filter->p + filter->r);
    filter->x = filter->x + filter->k * (measurement - filter->x);
    filter->p = (1 - filter->k) * filter->p;

    return filter->x;
}



