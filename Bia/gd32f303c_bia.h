/*!
  \file	 	gd32f303c_bia.h
  \brief	GD32F303C��Ӳ������
  \author	chuang
  \version	2024-07-17,for GD32F303CC  
*/

#ifndef __GD32F303C_BIA_H
#define __GD32F303C_BIA_H


/*	��C++�е���C���Դ���ʱ����ֹC++�������޸�C����
	C++�������ᶨ���cplusplus
	�����cplusplus�����壬������ʹ��C���Ե����ӹ淶
*/
#ifdef cplusplus
 extern "C" {
#endif

#include "gd32f30x.h"
#include "systick.h"
#include <stdio.h>
#include <string.h>

#define U8  unsigned char
#define S8  char
#define U16 unsigned short
#define S16 short
#define U32 unsigned int
#define S32 int
#define U64 unsigned long long
#define S64 long long
#define BOOL unsigned char

#define TRUE 1
#define FALSE 0
#define NULL 0

typedef U32 StackSize_t;  /* �����ڶ�ջ */

/*��̬ȫ�ֱ�������static����*/
/*�ⲿʹ�õ�ȫ�ֱ���g_��extern����*/

/*�ڲ����غ�������static����*/
/*�ⲿʹ�õ�ȫ�ֺ�����extern����*/


	 
typedef enum 
{
    COM1 = 0U,
    COM2 = 1U
}com_typedef_enum;
	 

#define COMn                             2U

#define EVAL_COM1                        USART0
#define EVAL_COM1_CLK                    RCU_USART0
#define EVAL_COM1_TX_PIN                 GPIO_PIN_9
#define EVAL_COM1_RX_PIN                 GPIO_PIN_10
#define EVAL_COM1_GPIO_PORT              GPIOA
#define EVAL_COM1_GPIO_CLK               RCU_GPIOA

#define EVAL_COM2                        USART1
#define EVAL_COM2_CLK                    RCU_USART1
#define EVAL_COM2_TX_PIN                 GPIO_PIN_2
#define EVAL_COM2_RX_PIN                 GPIO_PIN_3
#define EVAL_COM2_GPIO_PORT              GPIOA
#define EVAL_COM2_GPIO_CLK               RCU_GPIOA	 
	 
void gd_eval_com_init(com_typedef_enum com_n);
	 
#ifdef cplusplus
}
#endif

#endif  //GD32F303C_BIA_H

