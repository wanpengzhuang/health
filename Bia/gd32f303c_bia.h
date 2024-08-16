/*!
  \file	 	gd32f303c_bia.h
  \brief	GD32F303C的硬件配置
  \author	chuang
  \version	2024-07-17,for GD32F303CC  
*/

#ifndef __GD32F303C_BIA_H
#define __GD32F303C_BIA_H


/*	在C++中调用C语言代码时，防止C++编译器修改C代码
	C++编译器会定义宏cplusplus
	如果宏cplusplus被定义，就声明使用C语言的链接规范
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

typedef U32 StackSize_t;  /* 仅用于堆栈 */

/*静态全局变量，加static修饰*/
/*外部使用的全局变量g_，extern声明*/

/*内部本地函数，加static修饰*/
/*外部使用的全局函数，extern声明*/


	 
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

