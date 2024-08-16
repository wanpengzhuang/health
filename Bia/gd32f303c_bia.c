/*!
  \file	 	gd32f303c_bia.c
  \brief	GD32F303C的硬件配置
  \author	chuang
  \version	2024-07-17,for GD32F303CC
*/

#include "gd32f303c_bia.h"

static uint32_t USART_NUM[COMn] = {USART0, USART1};
static rcu_periph_enum COM_CLK[COMn] = {EVAL_COM1_CLK, EVAL_COM2_CLK};
static uint32_t COM_TX_PIN[COMn] = {EVAL_COM1_TX_PIN, EVAL_COM2_TX_PIN};
static uint32_t COM_RX_PIN[COMn] = {EVAL_COM1_RX_PIN, EVAL_COM2_RX_PIN};
static uint32_t COM_GPIO_PORT[COMn] = {EVAL_COM1_GPIO_PORT, EVAL_COM2_GPIO_PORT};
static rcu_periph_enum COM_GPIO_CLK[COMn] = {EVAL_COM1_GPIO_CLK, EVAL_COM2_GPIO_CLK};

/*!
	\brief		初始化配置USART
	\param[in]	com_n，串口的枚举
	  \arg		COM1（USART1）
	  \arg		COM2（USART2）
	\param[out]	无
	\retval		无

*/
void gd_eval_com_init(com_typedef_enum com_n)
{
    
    rcu_periph_clock_enable(COM_GPIO_CLK[com_n]);

    rcu_periph_clock_enable(COM_CLK[com_n]);

    gpio_init(COM_GPIO_PORT[com_n], GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, COM_TX_PIN[com_n]);

    gpio_init(COM_GPIO_PORT[com_n], GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, COM_RX_PIN[com_n]);

    /* USART configure */
    usart_deinit(USART_NUM[com_n]);
    usart_baudrate_set(USART_NUM[com_n], 115200U);
    usart_receive_config(USART_NUM[com_n], USART_RECEIVE_ENABLE);
    usart_transmit_config(USART_NUM[com_n], USART_TRANSMIT_ENABLE);
    usart_enable(USART_NUM[com_n]);
}
