
#ifndef __GD_I2C_H_
#define __GD_I2C_H_

#include <stdint.h>

#define I2C_STATUS_SUCCESS 0
#define I2C_STATUS_FAIL 1

void gd_i2c_init(void);

void gd_i2c_deinit(void);

uint8_t gd_i2c_read(uint8_t daddr, uint16_t regaddr, uint8_t* buff, uint16_t len, uint8_t addrlen);

uint8_t gd_i2c_write(uint8_t daddr, uint16_t regaddr, uint8_t* buff, uint8_t len, uint8_t addrlen);

uint8_t gd_i2c_send_cmddata(uint8_t daddr, uint8_t cmd, uint8_t data);

uint8_t gd_i2c_read_cmddata(uint8_t daddr, uint8_t cmd);

uint16_t gd_i2c_read_cmdword(uint8_t daddr, uint8_t cmd);

uint8_t gd_i2c_read_cmdspdata(uint8_t daddr, uint8_t cmd, uint8_t* buff, uint8_t len);
#endif
