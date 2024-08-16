/*!
  \file	 	main.c
  \brief	健康检测
  \author	chuang
  \version	2024-05-16,for GD32F303CC  
*/

/************************头文件**********************************/
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

//体温
#include "gd_i2c.h"
#include "mlx90614.h"

//RTOS
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
 
#include <math.h>



/************************宏定义**********************************/

#define SAMPLING_RATE_HZ 		13		//系统采样率为13Hz
#define CUTOFF_FREQUENCY_HZ 	5		//过滤得到5hz以下信号
#define WINDOW_SIZE 			20		//移动平均滤波器
#define HEART_RATE_THRESHOLD 	25 		//心跳幅度阈值
#define MAX_HEART_NUM 			5		//最大心率数据数量
#define MAX_BREATH_NUM 			5 		//最大呼吸率数据数量
#define MAX(a, b) ((a) > (b)? (a) : (b))
#define MIN(a, b) ((a) < (b)? (a) : (b))

//PPG
#define  IBI_array_size 10 	//RR间期数组10
#define  HRV_need_size  50 //计算HRV需要的数据数量300
#define MAX_BPM_ARRAY_NUM   5   //最大心率数据数量
#define PPG_HEART_RATE_THRESHOLD 	300  //心跳幅度阈值

/****************************结构体定义*************************************/

// 定义卡尔曼滤波器结构体
typedef struct 
{
    float q; // 过程噪声方差,越大越不相信模型预测值
    float r; // 测量噪声方差，越大越不相信新测量值的程度
    float x; // 估计值
    float p; // 估计值误差方差，越大越不相信当前预测状态
    float k; // 卡尔曼增益
} KalmanFilter;


// 定义结构体来保存高通滤波器状态
typedef struct 
{
    float lastInput; // 上一次输入值
    float lastOutput; // 上一次输出值
} HighPassFilter;



/************************全局变量定义*******************************/

//新建任务需要先定义任务句柄
TaskHandle_t start_task_Handler;
TaskHandle_t task1_Handler;
TaskHandle_t task2_Handler;
TaskHandle_t task3_Handler;

// 全局变量用于存储ADC值
volatile uint16_t g_adc_data = 0;

// 初始化过滤后的值
float filtered_value = 0; 
float pre_filtered_value = 0; 

// 得到新ADC数据的标志位
volatile bool flag_get_new_adc_data = false;

// 计算alpha (低通滤波器系数)
const float RC = 1.0f / (2 * 3.14159f * CUTOFF_FREQUENCY_HZ);
const float DT = 1.0f / SAMPLING_RATE_HZ;
const float ALPHA = DT / (RC + DT);

KalmanFilter kf;//定义卡尔曼滤波器

volatile bool rising_flag = false; // 上升标志
volatile bool falling_flag = false; // 下降标志
volatile float temp_valley = 0;//波谷
volatile float temp_peak = 0;//波峰
float heart_peak = 0;//心跳波峰
float temp_fuck = 0;
volatile int heart_time = 0;
int heart_rate = 0;
int avg_heart_rate = 0;
int heart_rate_sum = 0;
int get_herat_rate_num = 0;
int heart_rate_array[MAX_HEART_NUM];
int index = 0;//心率数组索引

//滑动窗口局部极大值检测呼吸
volatile int breath_time = 0;
int breath_rate = 0;
float breath_peak = 0;
float breath_valley = 0;

//呼吸检测
volatile int breath_rates[MAX_BREATH_NUM]; // 存储最近五次呼吸率
int breath_index = 0; // 用于索引最近五次呼吸率数组的变量

bool should_replace_min = true; // 用于交替标记下次替换最大还是最小值

volatile int bodymove_delay = 0;//延时显示体动


//体温检测**************************
float temp;//全局变量温度

//PPG
volatile int BPM;				//心率

volatile int Signal;		//采集到的原始信号值uint16_t 类型来保存大于1023的数
volatile int pre_signal;		//采集到的原始信号值
volatile  int IBI = 600;	//心跳间隔时间

int IBI_array[IBI_array_size] = {0}; //保存IBI值
int copy_IBI_data[HRV_need_size] = {0};
volatile int IBI_count = 0;
volatile int HRV_need_count = 0;//计算HRV的心率间期数组索引
unsigned long sampleCounter = 0;	//确定脉冲时间
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
volatile bool ppg_rising_flag = false; // 上升标志
volatile bool ppg_falling_flag = false; // 下降标志

int temp_IBI_array_change = 0;


/************************函数声明**********************************/

//定义任务
void start_task(void *pvParameters);

void task1(void *pvParameters);

void task2(void *pvParameters);

void task3(void *pvParameters);

void start_os(void);

//低通滤波
float low_pass_filter(float current_sample, float previous_filtered);

// 初始化卡尔曼滤波器
void kalman_init(KalmanFilter* filter, float q, float r, float init_x, float init_p) ;

// 更新卡尔曼滤波器状态
float kalman_update(KalmanFilter* filter, float measurement) ;

//初始化高通滤波器
void initFilter(HighPassFilter* filter) ;

//应用高通滤波器
float applyFilter(HighPassFilter* filter, float input, float dt, float cutoffFreq) ;

//心跳波峰检测,比较波峰波谷，波谷到波峰的幅度大于60判断为心跳导致的波峰
int find_peak(float adc_value, float pre_adc_value);

//更新呼吸
float update_breath(float new_sample) ;

//移动平均滤波器
uint16_t moving_average_filter(uint16_t new_sample) ;

//交替覆盖心率数组中的最大值最小值
void insert_alternate_min_max(int new_heart_rate);

//PPG**********************
void calculate_heart(void);

// 计算标准差SDNN
float calculate_SD(int data[], int n) ;

// 实现RMSSD函数
float calculate_RMSSD(int data[], int n);

// 实现计算 NN50 的函数
int calculate_nn50(int data[], int n);

// 实现 PANN_5 函数
float calculate_pnn50(int nn50_count, int total_beats) ;

// 计算HRV
float calculate_hrv(int rate[], int size) ;



/**************************主函数**********************************/
int main(void)
{

/**************************变量定义**********************************/
	
	
	
	
/***********************函数初始化**********************************/	
	
	// 初始化串口打印
	USART_Init();
	
	//延时函数初始化
	//delay_init() ;
	
	//配置系统滴答定时器,用于延时delay函数
    systick_config();
	
	// 初始化配置时钟
	rcu_config();
	
	//初始化配置ADC_GPIO
	adc_gpio_config();
	
	//初始化配置定时器
	timer_config();
	
	//初始化配置ADC
    adc_config();
	
	//PPG******************
	ppg_adc_config();
	//启动ADC的软件触发。
    adc_software_trigger_enable(ADC1, ADC_REGULAR_CHANNEL);
	//初始化定时器
	timer3_config();
	//PPG*****************
	
	//初始化卡尔曼滤波器
	kalman_init(&kf, 0.3f, 0.1f, 0.0f, 1.0f);
	
	//设置系统中断优先级分组只能选择第4组（只有抢占优先级，没有响应优先级）
	//初始化freertos，系统定时器中断频率为configTICK_RATE_HZ 120000000/1000
    systick_config();
	
	//启动freertos
	start_os();


	
/*************************主循环**********************************/	
	while(1);
	
}


/************************任务函数定义*******************************/

void start_os(void)
{
	xTaskCreate(start_task,"start_task",128,NULL,1,&start_task_Handler);
	vTaskStartScheduler();
}

void start_task(void *pvParameters)
{
	taskENTER_CRITICAL();
	
	//创建任务，越大越高
	xTaskCreate(task1,"task1",1024,NULL,3,&task1_Handler);
	xTaskCreate(task2,"task2",128,NULL,2,&task2_Handler);
	xTaskCreate(task3,"task3",128,NULL,1,&task3_Handler);
	vTaskDelete(start_task_Handler);
	taskEXIT_CRITICAL();
}

//每个任务都必须包含while（）和vTaskDelay来确定任务的执行周期
//vTaskDelay是相对定时，后面还有绝对定时函数vTaskDelayUntil()
void task1(void *pvParameters)
{
	int for_loop_num = 0;
	
	//定义高通滤波器
	HighPassFilter hpFilter;
	
	//初始化高通滤波器
	initFilter(&hpFilter);
	
	// 初始化心率数组
    for(for_loop_num = 0; for_loop_num < MAX_HEART_NUM; for_loop_num++) 
	{
        heart_rate_array[for_loop_num] = 0;
    }
	
	while(1)
	{		
		uint16_t adc_data_copy = 0;
		
		//PPG**************
		printf("signal = %u\r\n",Signal);//原始信号
		if(BPM>50 && BPM<100)
		{
			printf("BPM = %d\r\n",BPM);//心率
			printf("IBI = %d\r\n",IBI);//心跳间隔
//			if(temp_IBI_array_change != IBI_array[IBI_array_size-1])
//			{
//				//HRV参数计算,数组已满且有效IBI值
//				calculate_hrv(IBI_array, IBI_array_size);		
//				temp_IBI_array_change = IBI_array[IBI_array_size-1];

//			}
		}	
		//PPG*************		
	
		if(flag_get_new_adc_data)
		{
			
			// 禁用中断或使用其他同步机制来创建临界区，防止中断打断
			//进入临界区，临界区代码要精简
			taskENTER_CRITICAL();
			
            // 复制全局变量以避免竞态条件
            adc_data_copy = g_adc_data;
            
            // 清除新数据标志
            flag_get_new_adc_data = false;
            
            // 重新启用中断或结束同步区域
			//重新启用中断或结束同步区域，退出临界区
			taskEXIT_CRITICAL();
			
			//保存上一个值
			pre_filtered_value = filtered_value;
			
			//滤波处理
			//将ADC值转换为浮点型并应用低通滤波器
			filtered_value = low_pass_filter((float)adc_data_copy, filtered_value);	
			
			//呼吸检测
			update_breath(filtered_value);
			//上位机观察
			//printf("pressure=%d\n", (int)filtered_value);
			
			//应用高通滤波器，假设采样间隔为0.08秒，截止频率为0.8Hz
			filtered_value = applyFilter(&hpFilter, filtered_value, 0.08f, 0.8f);
			
			//体动检测
			if(filtered_value > 100)
			{
				bodymove_delay = 30;
			}
			if(bodymove_delay)
			{
				printf("bodymove=1\r\n");
				bodymove_delay--;
			}

			
			//上位机观察
			//printf("pressure=%d\n", (int)filtered_value);
			
			
			//*5放大
			filtered_value = filtered_value*5;
			
			//卡尔曼滤波平滑处理
			//filtered_value = kalman_update(&kf, filtered_value);
				
			//心率检测
			heart_rate = find_peak(filtered_value,pre_filtered_value);	
				
			// 上位机观察
			printf("pressure=%d\r\n", (int)filtered_value);
			
			if(heart_rate>50 && heart_rate<100)
			{
				//HRV检测
//				hrv_sum += abs(heart_rate - pre_heart_rate)
//				hrv_num++;
//				hrv_level = hrv_sum/hrv_num;
				
				//get_herat_rate_num++;
				//heart_rate_sum += heart_rate;
				
				//插入心率数据到数组
				//heart_rate_array[index] = heart_rate;
				//循环覆盖最大最小值覆盖
				insert_alternate_min_max(heart_rate);
				
				// 使用模运算确保索引在数组范围内
				index = (index + 1) % MAX_HEART_NUM; 
				
				//求心率数组平均值
				heart_rate_sum = 0;
				for( for_loop_num=0;for_loop_num<MAX_HEART_NUM;for_loop_num++)
				{
					heart_rate_sum += heart_rate_array[for_loop_num];
				}
				
				//计算平均心率
				if(heart_rate_array[MAX_HEART_NUM-1])
				{
					avg_heart_rate = heart_rate_sum/MAX_HEART_NUM;
					
					printf("Heart rate=%d\r\n",avg_heart_rate);
				}
			}
			
		}	

		//延时100ms,此时会被其他任务抢占
		vTaskDelay(10);
	}
	
}


void task2(void *pvParameters)
{
	//IIC初始化
	gd_i2c_init();

	//mlx90614初始化红外测温
	mlx90614_Init(); 
	
	while(1)
	{
		//真实温度，额头2合适，后颈2.5合适
		float real_temp = SMBus_ReadTemp();
		
		//小于39时，额温枪补偿
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
		//printf("signal = %u\r\n",Signal);//原始信号
		if(BPM>50 && BPM<100)
		{
			//printf("BPM = %d\r\n",BPM);//心率
			//printf("IBI = %d\r\n",IBI);//心跳间隔
			if(temp_IBI_array_change != IBI_array[IBI_array_size-1])
			{
				//HRV参数计算,数组已满且有效IBI值
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
	//const char command[] = "\x8A"; // 正确的十六进制命令
	
	//初始化惊帆串口
	USART1_init(38400U);
	
	//发送启动命令给串口模块
	send_at_command("\x8A");
	
	while(1)
	{

		vTaskDelay(100);
	}
	
}

/*************************函数定义*************************************/
// 计算标准差SDNN 
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

	// 这里使用n-1作为分母，因为这是样本标准差
    return sqrt(standardDeviation / (n-1)); 
}

// 实现RMSSD函数
float calculate_RMSSD(int data[], int n) 
{
    float rmssd = 0.0;
    int i;
    
    for (i = 0; i < n - 1; ++i) { // 注意这里循环到n-2，因为我们比较的是相邻间隔
        rmssd += pow(data[i + 1] - data[i], 2);
    }
    
    return sqrt(rmssd / (n - 1)); // 注意这里使用n-1作为分母
}

// 实现计算 NN50 的函数
int calculate_NN50(int data[], int n) 
{
    //static int NN50 = 0;
    int i;
    
    for (i = 0; i < n - 1; ++i) { // 同样地，循环到n-2
        if (abs(data[i + 1] - data[i]) > 50)
            NN50++;
    }
    
    return NN50;
}

// 实现 PNN50 函数
float calculate_PNN50(int nn50_count, int total_beats) 
{
   return ((float)nn50_count / total_beats) * 100.0f; // 转换成百分比形式
}



// 计算HRV
float calculate_hrv(int IBI_data[], int size) 
{
    int i = 0;
	//static int NN_total = 0;
	
	//计算总RR间期数量
	NN_total += size;
	
	//计算NN50
	calculate_NN50(IBI_data, size); 
	PNN50 = calculate_PNN50(NN50, NN_total);
	printf("NN50 = %d\r\n",(int)NN50);//NN50
	printf("PNN50 = %d\r\n",(int)PNN50);//PNN50
	
    for ( i = 0; i < IBI_array_size; ++i) 
	{
		copy_IBI_data[HRV_need_count] = IBI_data[i]; 
		
		//更新索引
		HRV_need_count++;
		if(HRV_need_count == HRV_need_size)
		{
			HRV_need_count = 0;
		}
    }
	
	//大于600用于过滤掉第一次数组数据
	if(copy_IBI_data[HRV_need_size-1] && copy_IBI_data[0]>600)
	{
		pre_SDNN = SDNN;
		pre_RMSSD = RMSSD;
		
		// 使用标准差SDNN
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

//计算心跳
void calculate_heart(void)
{
	//清除ADC的结束转换（End of Conversion）标志。
	adc_flag_clear(ADC1, ADC_FLAG_EOC);
	
	// 等待ADC的转换完成。
	while(SET != adc_flag_get(ADC1, ADC_FLAG_EOC));

	//保存上一个ADC值
	pre_signal = Signal;
	
	//读取ADC的值，AD值右移两位，用来数据保持一致性
	Signal = ADC_RDATA(ADC1)>>2;
	
	//更新时间
	sampleCounter += 2;


	if(pre_signal < Signal)
	{
		//如果之前一直在下降，现在突然上升代表这里为波谷
		if(ppg_falling_flag)
		{
			//更新脉搏波波谷
			ppg_valley = pre_signal;

			//计算波峰到波谷（下降沿）
			//如果也得到了波峰，则计算下降沿幅度大于心跳阈值为心跳
			if(ppg_peak && abs(ppg_peak - ppg_valley)>PPG_HEART_RATE_THRESHOLD)
			{

				//更新心跳间隔,过滤异常间期数据
				if((60000 / sampleCounter > 50) && (60000 / sampleCounter <120))
				{
					IBI = sampleCounter;
					//BPM = 60000 / sampleCounter;
				}

				//重置时间
				sampleCounter = 0;

				//重置波谷波峰
				ppg_valley = 0;
				ppg_peak = 0;
			}
		}

		ppg_rising_flag = true;
        ppg_falling_flag = false;

	}


	if(pre_signal > Signal)
	{
		//如果之前一直在上升，现在突然下降代表这里为波峰
		if(ppg_rising_flag)
		{
			//更新脉搏波波峰
			ppg_peak = pre_signal;

			// //计算波谷到波峰（上升沿）
			// //如果也得到了波谷，则计算上升沿幅度大于心跳阈值为心跳
			// if(ppg_valley && abs(ppg_peak - ppg_valley)>PPG_HEART_RATE_THRESHOLD)
			// {

			// 	//更新心跳间隔,过滤异常间期数据
			// 	if((60000 / sampleCounter > 50) && (60000 / sampleCounter <120))
			// 	{
			// 		IBI = sampleCounter;
			// 		BPM = 60000 / sampleCounter;
			// 	}

			// 	//重置时间
			// 	sampleCounter = 0;

			// 	//重置波谷波峰
			// 	ppg_valley = 0;
			// 	ppg_peak = 0;
			// }
		}

		ppg_rising_flag = false;
        ppg_falling_flag = true;

	}

	//使用sampleCounter判断是否得到新IBI	
	if(!sampleCounter)
	{
		//循环数组保存IBI
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

		//计算心率
		if(IBI_array[IBI_array_size - 1])
		{
			runningTotal = 0;
			for( i = 0;i<IBI_array_size;i++)
			{
				runningTotal += IBI_array[i];
			}
			//IBI求平均
			runningTotal /= IBI_array_size;			
			
			//求心率
			BPM = 60000/runningTotal ;
		}
	}
	
	//超时重置时间
	if(sampleCounter>1200)
	{
		sampleCounter = 0;
	}
}

//定时器3的中断服务函数
//只在ISR中设置必要的状态或者标志位，然后在主循环或其他任务里处理这些状态。
void TIMER3_IRQHandler(void)
{
    if(timer_interrupt_flag_get(TIMER3, TIMER_INT_UP) != RESET)
    {
		//设置标志位在主循环中执行处理逻辑
		 //get_adc_data_flag = 1;
		//ADC信号处理
		calculate_heart();
		
		// 清除定时器更新中断标志
		timer_interrupt_flag_clear(TIMER3, TIMER_INT_UP);
		
	}
}



//交替覆盖心率数组中的最大值最小值
void insert_alternate_min_max(int new_heart_rate) 
{
    int *target_index_ptr; // 指向要替换的数组索引的指针
    int max_val = heart_rate_array[0];
    int min_val = heart_rate_array[0];
    int max_index = 0;
    int min_index = 0;
	int i = 0;

    // 遍历数组找到最大值和最小值及其索引
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

    // 根据标记选择替换最大值或者最小值
    if(should_replace_min || (new_heart_rate > avg_heart_rate) || (new_heart_rate < avg_heart_rate-15)) //
	{
        target_index_ptr = &min_index;
		// 替换目标索引处的值
		heart_rate_array[*target_index_ptr] = new_heart_rate;
    } 	
//	else if(new_heart_rate < avg_heart_rate-15)
//	{
//		//不插入数据，舍弃
//	}	 
	else
	{
        target_index_ptr = &max_index;
		// 替换目标索引处的值
		heart_rate_array[*target_index_ptr] = new_heart_rate;
    }

    // 替换目标索引处的值
    //heart_rate_array[*target_index_ptr] = new_heart_rate;

	if(heart_rate_array[MAX_HEART_NUM-1])
	{
		// 切换标记以便下次替换另一个值
		should_replace_min = !should_replace_min;
	}
}


//移动平均滤波器
uint16_t moving_average_filter(uint16_t new_sample) 
{
	//静态局部变量只初始化一次
	static float window_sum = 0;
	static int window_index = 0;
	static float window[WINDOW_SIZE] = {0};
	
    // 从总和中减去最旧的样本值
    window_sum -= window[window_index];
    
    // 将新的样本值加到总和中，并将其添加到滑动窗口数组中
    window_sum += new_sample;
    window[window_index] = new_sample;
    
    // 更新索引，如果超过了数组边界，则回到数组开始处（实现循环）
    window_index = (window_index + 1) % WINDOW_SIZE;

    // 返回平均值作为过滤结果
    return window_sum / WINDOW_SIZE;
}

//呼吸检测
float update_breath(float new_sample) 
{
	float temp_breath_data = 0;
	//float temp_breath_rate = 0;
	static float pre_breath_data = 0;
	volatile static bool start_find_breath_flag = 0;
	int loop_num = 0;
	float average_breath_rate = 0;
	float breath_sum = 0;
	
	//移动平均滤波
	temp_breath_data = moving_average_filter(new_sample);
	
	//上位机观察
	//printf("pressure=%d\n", (int)temp_breath_data);
	
	//波峰判断
	if(!start_find_breath_flag && pre_breath_data < temp_breath_data)
	{
		//开始检测呼吸波峰标志位
		start_find_breath_flag = 1;
		
		//更新波谷
		breath_valley = pre_breath_data;
		
		//为防止按照两波峰计算中间被打断，只取一般也就是波谷到波峰
		//重置时间
		breath_time = 0;
	}
	
	//可能需要根据波峰设置阈值筛选非呼吸波峰
	if(start_find_breath_flag && (pre_breath_data > temp_breath_data) )
	{
		breath_peak = pre_breath_data;
		
		//判断是不是呼吸
		if((pre_breath_data - breath_valley) > 40 && (pre_breath_data - breath_valley) < 300 && breath_time>500)
		{
			//得到呼吸波峰
			breath_rate = 60000/(breath_time*2);
			
			// 将新的呼吸率添加到最近五次呼吸率数组中
            breath_rates[breath_index] = breath_rate;
            
            // 更新索引
            breath_index = (breath_index + 1) % MAX_BREATH_NUM;
			
			// 计算最近五次平均呼吸率
			for (loop_num = 0; loop_num < MAX_BREATH_NUM; ++loop_num) 
			{
				breath_sum += breath_rates[loop_num];
			}
			
			if(breath_rates[MAX_BREATH_NUM-1])
			{
				//求出平均呼吸率
				average_breath_rate = breath_sum / MAX_BREATH_NUM;
				if(average_breath_rate<25)
				{
					printf("Breath rate=%d\r\n",(int)average_breath_rate);
			
				}
			}

		}
		
		
		
		//重置开始检测呼吸标志位
		start_find_breath_flag = 0;
	}
	
	//保存上一个移动平均滤波值
	pre_breath_data = temp_breath_data;
	
	return average_breath_rate;

}

//心跳波峰检测,比较波峰波谷，波谷到波峰的幅度大于心跳阈值判断为心跳导致的波峰
int find_peak(float adc_value, float pre_adc_value)
{
	int temp_heart_rate = 0;
	
	if(adc_value > pre_adc_value)
    {
		//如果之前一直在下降，现在突然上升代表这里为波谷
		if(falling_flag)
		{ 
			//temp_valley = adc_value;
			//波谷应该是上一个点
			temp_valley = pre_adc_value;
			
			//计算波峰到波谷（下降沿）
			//如果也得到了波谷，则计算下降沿幅度大于心跳阈值为心跳
			if(temp_peak && abs(temp_peak - temp_valley)>HEART_RATE_THRESHOLD)
			{
				temp_fuck = abs(temp_valley -temp_peak);
				temp_heart_rate = 60000 / heart_time;
				
				//得到心跳波峰
				heart_peak = temp_peak;
				
				//重新计算时间
				heart_time = 0;
			}
		}
		
        rising_flag = true;
        falling_flag = false;
    }
	
    if(adc_value < pre_adc_value)
    {
		//如果之前一直在上升，现在突然下降代表这里为波峰
		if(rising_flag)
		{ 
			//temp_peak = adc_value;
			//波峰应该是上一个点
			temp_peak = pre_adc_value;
			
//			//计算波谷到波峰（上升沿）
//			//如果也得到了波峰，则计算上升沿幅度大于心跳阈值为心跳
//			if(temp_valley && abs(temp_peak - temp_valley)>HEART_RATE_THRESHOLD)
//			{
//				temp_fuck = abs(temp_valley -temp_peak);
//				temp_heart_rate = 60000 / heart_time;
//				
//				//得到心跳波峰
//				heart_peak = temp_peak;
//				
//				//重新计算时间
//				heart_time = 0;
//			}		
		}
		
        falling_flag = true;
        rising_flag = false;
    }
	
	return temp_heart_rate;

}


//一阶IIR低通滤波器
float low_pass_filter(float current_sample, float previous_filtered) 
{
    return ALPHA * current_sample + (1 - ALPHA) * previous_filtered;
}



// 初始化高通滤波器
void initFilter(HighPassFilter* filter) 
{
    filter->lastInput = 0.0f;
    filter->lastOutput = 0.0f;
}



// 应用高通滤波器
float applyFilter(HighPassFilter* filter, float input, float dt, float cutoffFreq) 
{
    float RC = 1.0f / (2 * 3.14159265358979323846 * cutoffFreq);
    float alpha = dt / (RC + dt);
    float output = alpha * (filter->lastOutput + input - filter->lastInput);
    
    filter->lastInput = input;
    filter->lastOutput = output;
    
    return output;
}



// 初始化卡尔曼滤波器
void kalman_init(KalmanFilter* filter, float q, float r, float init_x, float init_p) 
{
    filter->q = q;
    filter->r = r;
    filter->x = init_x;
    filter->p = init_p;
}


// 更新卡尔曼滤波器状态
float kalman_update(KalmanFilter* filter, float measurement) 
{
    // 预测步骤
    filter->p = filter->p + filter->q;

    // 更新步骤
    filter->k = filter->p / (filter->p + filter->r);
    filter->x = filter->x + filter->k * (measurement - filter->x);
    filter->p = (1 - filter->k) * filter->p;

    return filter->x;
}



