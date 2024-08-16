#ifndef __UART_H
#define __UART_H

#include "gd32f30x.h"
#include "systick.h"
#include <stdio.h>
#include <string.h>


int fputc(int ch, FILE *f);
void USART_Init(void);
void USART_PrintString(const char *str);


//¾ª·«*******
void USART1_init(uint32_t bound);
void send_at_command(const char* command);

#endif
