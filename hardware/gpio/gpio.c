/*
  文	件: gpio.c
  作	者: 庄万鹏
  创建日期: 2024-05-16
  版	本：for GD32F303CC
  
  描	述：初始化配置GPIO端口引脚
	  
  修改时间：2024-05-16
	1.增加初始化配置GPIO例程
  
  
*/


/************************头文件**********************************/
#include "gpio.h"


void gpio_config(void)
{
    //配置端口模式为模拟输入，端口速度为最大速度
    //gpio_init(GPIOA, GPIO_MODE_AIN, GPIO_OSPEED_MAX, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);
	
}

