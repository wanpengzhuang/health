/*!
  \file	 	uart.c
  \brief	���ڴ��ڴ�ӡ�ض�����д�ӡ���
  \author	chuang
  \version	2024-05-16,for GD32F303CC  
*/


/************************ͷ�ļ�**********************************/
#include "uart.h"


/************************�궨��**********************************/
#define USART_PORT 			USART0		//���ں�
#define USART_GPIO_PORT 	GPIOA		//���Ŷ˿�
#define USART_RCU_PORT 		RCU_USART0	//����ʱ��
#define USART_RCU_GPIO_PORT RCU_GPIOA	//���Ŷ˿�ʱ��
#define USART_TX_PIN 		GPIO_PIN_9			
#define USART_RX_PIN 		GPIO_PIN_10
#define USART_BAUDRATE 		115200U
#define USART_WORD_LENGTH 	USART_WL_8BIT  //����֡����
#define USART_STOP_BIT 		USART_STB_1BIT //ֹͣλ����
#define USART_PARITY 		USART_PM_NONE  //��żУ��λ



/************************��������**********************************/
// ��дfputc����
int fputc(int ch, FILE *f) 
{
    // �ȴ�USART���ͻ�����Ϊ��
    while (usart_flag_get(USART_PORT, USART_FLAG_TBE) == RESET);
    // �����ַ�
    usart_data_transmit(USART_PORT, (uint32_t)ch);
    while (usart_flag_get(USART_PORT, USART_FLAG_TC) == RESET);

    return ch;
}

// ��ʼ��USART
void USART_Init(void) 
{
   /* ʹ��GPIO�˿ڣ���ָ����PIN��Ϊ���� */
   rcu_periph_clock_enable(USART_RCU_GPIO_PORT);

   /*ʹ�ܴ��ڵ�ʱ�� */
   rcu_periph_clock_enable(USART_RCU_PORT);

   /*����USARTx_TxΪ�����������*/
   gpio_init(USART_GPIO_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, USART_TX_PIN);

   /*����USARTx_RxΪ�������� */
   gpio_init(USART_GPIO_PORT, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, USART_RX_PIN);

   /* USART ���� */
   usart_deinit(USART_PORT);//���ô���
   usart_baudrate_set(USART_PORT, USART_BAUDRATE);//���ô��ڵĲ�����
   usart_word_length_set(USART_PORT, USART_WORD_LENGTH);         // ֡�����ֳ�
   usart_stop_bit_set(USART_PORT, USART_STOP_BIT);              // ֹͣλ1λ
   usart_parity_config(USART_PORT, USART_PARITY);          // ����żУ��λ
   usart_receive_config(USART_PORT, USART_RECEIVE_ENABLE);//ʹ�ܽ�����
   usart_transmit_config(USART_PORT, USART_TRANSMIT_ENABLE);//ʹ�ܷ�����
   usart_enable(USART_PORT);//ʹ��USART
}

// ��ӡ�ַ���
void USART_PrintString(const char *str) 
{
    while (*str) 
	{
        // �ȴ�USART���ͻ�����Ϊ��
        while (usart_flag_get(USART_PORT, USART_FLAG_TBE) == RESET);
        // �����ַ�
        usart_data_transmit(USART_PORT, (uint32_t)*str++);
        while (usart_flag_get(USART_PORT, USART_FLAG_TC) == RESET);
    }
}


//����************************************
#define JINGFAN_BUFFER_SIZE   100 
uint8_t jingfan_txbuffer[JINGFAN_BUFFER_SIZE];
uint8_t jingfan_rxbuffer[JINGFAN_BUFFER_SIZE];
//__IO�����õ���volatile��ֱ�Ӵ��ڴ��ȡ�����Ż��ñ���
__IO uint16_t jingfan_txcount = 0; 
__IO uint16_t jingfan_rxcount = 0; 

//��ʼ��USART1��PA2=TX��PA3=RX������������ATָ���MAX30102
void USART1_init(uint32_t bound)//9600U
{
	/* ��ʼ��GPIO */
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_USART1);
	
	// ����USART1��TX���ţ�PA2��Ϊ�����������
    gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_2);
	
	// ����USART3��TX���ţ�PA3��Ϊ��������
    gpio_init(GPIOA, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_3);
    
    /* ��ʼ��UART */
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
	
	/* ���ý����ж�,�����жϲŻ�����жϷ����� */
    usart_interrupt_enable(USART1, USART_INT_RBNE);
	
	/* �����ж����ȼ� */
    nvic_priority_group_set(NVIC_PRIGROUP_PRE1_SUB3);
    nvic_irq_enable(USART1_IRQn, 1, 0);//��1��ĵ�0�����ȼ�
	
}



//��USART1����ATָ��
void send_at_command(const char* command)
{
	uint16_t i;
	
    /* ������Ƶ����ͻ����� */
    while ((*command != '\0') && (jingfan_txcount < JINGFAN_BUFFER_SIZE))
    {
        jingfan_txbuffer[jingfan_txcount++] = *command++;
    }

    /* �������� */
    for (i = 0; i < jingfan_txcount; i++)
    {
        usart_data_transmit(USART1, jingfan_txbuffer[i]);
        while (usart_flag_get(USART1, USART_FLAG_TBE) == RESET);
    }
	
	 /* �ȴ�������ȫ������� */
    while (usart_flag_get(USART1, USART_FLAG_TC) == RESET);

    /* ��շ��ͻ����� */
    jingfan_txcount = 0;
}


//�жϷ��������մ��������ص�����
void USART1_IRQHandler(void)
{
	int i;
    /* ������յ����� */
    if (usart_interrupt_flag_get(USART1, USART_INT_FLAG_RBNE) != RESET)
    {
        /* �����ݶ�ȡ�����ջ����� */
        if (jingfan_rxcount < JINGFAN_BUFFER_SIZE - 1) // ȷ���пռ����null�ַ�
        {
			uint8_t data = usart_data_receive(USART1);
			
			if(data == 255)
			{
				jingfan_rxcount = 0;  // ���ü�����
				memset(jingfan_rxbuffer, 0, sizeof(jingfan_rxbuffer));  // ��ջ�����
				jingfan_rxbuffer[jingfan_rxcount++] = data;
				
			}
			else if(jingfan_rxbuffer[0] == 255)
			{	
				jingfan_rxbuffer[jingfan_rxcount++] = data;
			}

			
			//�յ�һ֡���ݴ�ӡ
			if(jingfan_rxbuffer[87])
			{
				//data_received_flag = 1;
				//����ֱ�Ӵ�ӡ�ַ��������ݻ������Ҫһλλ��ӡ�ַ�
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
            jingfan_rxbuffer[jingfan_rxcount] = '\0';  // ����ַ�����ֹ��
            jingfan_rxcount = 0;  // ���ü�����
        }
    }
}
