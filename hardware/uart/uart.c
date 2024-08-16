/*!
  \file	 	uart.c
  \brief	用于串口打印重定向进行打印输出
  \author	chuang
  \version	2024-05-16,for GD32F303CC  
*/


/************************头文件**********************************/
#include "uart.h"


/************************宏定义**********************************/
#define USART_PORT 			USART0		//串口号
#define USART_GPIO_PORT 	GPIOA		//引脚端口
#define USART_RCU_PORT 		RCU_USART0	//串口时钟
#define USART_RCU_GPIO_PORT RCU_GPIOA	//引脚端口时钟
#define USART_TX_PIN 		GPIO_PIN_9			
#define USART_RX_PIN 		GPIO_PIN_10
#define USART_BAUDRATE 		115200U
#define USART_WORD_LENGTH 	USART_WL_8BIT  //数据帧长度
#define USART_STOP_BIT 		USART_STB_1BIT //停止位数量
#define USART_PARITY 		USART_PM_NONE  //奇偶校验位



/************************函数定义**********************************/
// 重写fputc函数
int fputc(int ch, FILE *f) 
{
    // 等待USART发送缓冲区为空
    while (usart_flag_get(USART_PORT, USART_FLAG_TBE) == RESET);
    // 发送字符
    usart_data_transmit(USART_PORT, (uint32_t)ch);
    while (usart_flag_get(USART_PORT, USART_FLAG_TC) == RESET);

    return ch;
}

// 初始化USART
void USART_Init(void) 
{
   /* 使能GPIO端口，用指定的PIN作为串口 */
   rcu_periph_clock_enable(USART_RCU_GPIO_PORT);

   /*使能串口的时钟 */
   rcu_periph_clock_enable(USART_RCU_PORT);

   /*配置USARTx_Tx为复用推挽输出*/
   gpio_init(USART_GPIO_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, USART_TX_PIN);

   /*配置USARTx_Rx为浮空输入 */
   gpio_init(USART_GPIO_PORT, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, USART_RX_PIN);

   /* USART 配置 */
   usart_deinit(USART_PORT);//重置串口
   usart_baudrate_set(USART_PORT, USART_BAUDRATE);//设置串口的波特率
   usart_word_length_set(USART_PORT, USART_WORD_LENGTH);         // 帧数据字长
   usart_stop_bit_set(USART_PORT, USART_STOP_BIT);              // 停止位1位
   usart_parity_config(USART_PORT, USART_PARITY);          // 无奇偶校验位
   usart_receive_config(USART_PORT, USART_RECEIVE_ENABLE);//使能接收器
   usart_transmit_config(USART_PORT, USART_TRANSMIT_ENABLE);//使能发送器
   usart_enable(USART_PORT);//使能USART
}

// 打印字符串
void USART_PrintString(const char *str) 
{
    while (*str) 
	{
        // 等待USART发送缓冲区为空
        while (usart_flag_get(USART_PORT, USART_FLAG_TBE) == RESET);
        // 发送字符
        usart_data_transmit(USART_PORT, (uint32_t)*str++);
        while (usart_flag_get(USART_PORT, USART_FLAG_TC) == RESET);
    }
}


//惊帆************************************
#define JINGFAN_BUFFER_SIZE   100 
uint8_t jingfan_txbuffer[JINGFAN_BUFFER_SIZE];
uint8_t jingfan_rxbuffer[JINGFAN_BUFFER_SIZE];
//__IO的作用等于volatile，直接从内存读取，不优化该变量
__IO uint16_t jingfan_txcount = 0; 
__IO uint16_t jingfan_rxcount = 0; 

//初始化USART1（PA2=TX，PA3=RX），用来发送AT指令给MAX30102
void USART1_init(uint32_t bound)//9600U
{
	/* 初始化GPIO */
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_USART1);
	
	// 设置USART1的TX引脚（PA2）为复用推挽输出
    gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_2);
	
	// 设置USART3的TX引脚（PA3）为浮空输入
    gpio_init(GPIOA, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_3);
    
    /* 初始化UART */
    usart_deinit(USART1);
    usart_baudrate_set(USART1, bound);
    usart_word_length_set(USART1, USART_WL_8BIT);
    usart_stop_bit_set(USART1, USART_STB_1BIT);
    usart_parity_config(USART1, USART_PM_NONE);
    usart_hardware_flow_rts_config(USART1, USART_RTS_DISABLE);
    usart_hardware_flow_cts_config(USART1, USART_CTS_DISABLE);
    usart_receive_config(USART1, USART_RECEIVE_ENABLE);
    usart_transmit_config(USART1, USART_TRANSMIT_ENABLE);
    usart_enable(USART1);
	
	/* 启用接收中断,启动中断才会调用中断服务函数 */
    usart_interrupt_enable(USART1, USART_INT_RBNE);
	
	/* 设置中断优先级 */
    nvic_priority_group_set(NVIC_PRIGROUP_PRE1_SUB3);
    nvic_irq_enable(USART1_IRQn, 1, 0);//第1组的第0级优先级
	
}



//给USART1发送AT指令
void send_at_command(const char* command)
{
	uint16_t i;
	
    /* 将命令复制到发送缓冲区 */
    while ((*command != '\0') && (jingfan_txcount < JINGFAN_BUFFER_SIZE))
    {
        jingfan_txbuffer[jingfan_txcount++] = *command++;
    }

    /* 发送数据 */
    for (i = 0; i < jingfan_txcount; i++)
    {
        usart_data_transmit(USART1, jingfan_txbuffer[i]);
        while (usart_flag_get(USART1, USART_FLAG_TBE) == RESET);
    }
	
	 /* 等待数据完全发送完毕 */
    while (usart_flag_get(USART1, USART_FLAG_TC) == RESET);

    /* 清空发送缓冲区 */
    jingfan_txcount = 0;
}


//中断服务函数接收传感器返回的数据
void USART1_IRQHandler(void)
{
	int i;
    /* 如果接收到数据 */
    if (usart_interrupt_flag_get(USART1, USART_INT_FLAG_RBNE) != RESET)
    {
        /* 将数据读取到接收缓冲区 */
        if (jingfan_rxcount < JINGFAN_BUFFER_SIZE - 1) // 确保有空间放置null字符
        {
			uint8_t data = usart_data_receive(USART1);
			
			if(data == 255)
			{
				jingfan_rxcount = 0;  // 重置计数器
				memset(jingfan_rxbuffer, 0, sizeof(jingfan_rxbuffer));  // 清空缓冲区
				jingfan_rxbuffer[jingfan_rxcount++] = data;
				
			}
			else if(jingfan_rxbuffer[0] == 255)
			{	
				jingfan_rxbuffer[jingfan_rxcount++] = data;
			}

			
			//收到一帧数据打印
			if(jingfan_rxbuffer[87])
			{
				//data_received_flag = 1;
				//不能直接打印字符串，数据会错误，需要一位位打印字符
				for(i=0;i<88;i++)
				{
					if(i==66)//test
						//printf("%c", rxbuffer[i]);
						printf("SPO2 = %d\r\n", (int)jingfan_rxbuffer[i]);//test
				}

			}
        }
		
		if (jingfan_rxcount == JINGFAN_BUFFER_SIZE - 1)
        {
			
			printf("Error: rxbuffer is BOOM\r\n");
            jingfan_rxbuffer[jingfan_rxcount] = '\0';  // 添加字符串终止符
            jingfan_rxcount = 0;  // 重置计数器
        }
    }
}
