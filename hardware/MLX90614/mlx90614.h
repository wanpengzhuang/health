#ifndef __MLX90614_H
#define __MLX90614_H

/* Includes ------------------------------------------------------------------*/
#include "stdlib.h"	 
#include "gd32f30x.h"
#include "gd_i2c.h"


#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t

void mlx90614_Init(void);
u16 SMBus_ReadMemory(u8, u8);
u8 PEC_Calculation(u8*);
float SMBus_ReadTemp(void); 

#endif
