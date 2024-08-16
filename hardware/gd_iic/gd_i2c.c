
#include "gd_i2c.h"
#include "gd32f30x_i2c.h"
// #include "rtthread.h"

#include <string.h>

#define USE_I2C_OUTTIME_RETURN 	1 		// I2C超时返回
#define I2C0_SPEED              50000	//原版本高速400000,GY906只能50000
#define I2C0_SLAVE_ADDRESS7     0xA0
#define I2C_PAGE_SIZE           8

static uint8_t i2c_onceflag = 0;

/**
 * @brief 微秒延时
 *
 * @param us
 */
void gd_i2c_delayus(uint32_t us)
{
    while (us--) {
        __nop();
        __nop();
        __nop();
        __nop();
        __nop();
    }
}

void gd_i2c_delayms(uint32_t ms)
{
    gd_i2c_delayus(1000 * ms);
}
#define I2C_DELAYMS gd_i2c_delayms
/**
 * @brief 检查标志位
 *
 * @param flag 外设标志类型
 * @param state 状态
 * @return uint8_t I2C_STATUS_SUCCESS=成功 I2C_STATUS_FAIL=失败
 */
uint8_t gd_i2c_flag_check(i2c_flag_enum flag, uint16_t state)
{
    uint16_t count = 0;
    while (i2c_flag_get(I2C0, flag) == state) {
        if (count++ > 30000) {
            gd_i2c_deinit();
            gd_i2c_init();
            return I2C_STATUS_FAIL;
        }
        gd_i2c_delayus(1);
    }
    return I2C_STATUS_SUCCESS;
}

/**
 * @brief I2C外设初始化
 *
 */
void gd_i2c_init(void)
{
    if (i2c_onceflag == 0) {
        i2c_deinit(I2C0);

        rcu_periph_clock_enable(RCU_GPIOB); 
        gpio_init(GPIOB, GPIO_MODE_AF_OD, GPIO_OSPEED_50MHZ, GPIO_PIN_6 | GPIO_PIN_7);

        rcu_periph_clock_enable(RCU_I2C0);
		i2c_clock_config(I2C0, I2C0_SPEED, I2C_DTCY_2);
        /* configure I2C0 address */
        i2c_mode_addr_config(I2C0, I2C_I2CMODE_ENABLE, I2C_ADDFORMAT_7BITS, I2C0_SLAVE_ADDRESS7);//地址0

        /* enable I2C0 */
        i2c_enable(I2C0);
        i2c_ack_config(I2C0, I2C_ACK_ENABLE);
        i2c_onceflag = 1;
    }
    /* enable acknowledge */
}

/**
 * @brief I2C外设去初始化
 *
 */
void gd_i2c_deinit(void)
{
    i2c_disable(I2C0);
    i2c_deinit(I2C0);
    rcu_periph_clock_disable(RCU_I2C0);
    i2c_onceflag = 0;
}

/**
 * @brief I2C读取
 *
 * @param daddr 设备地址
 * @param regaddr 寄存器地址
 * @param buff 缓冲
 * @param len  长度
 * @param addrlen 寄存器地址长度
 * @return uint8_t I2C_STATUS_SUCCESS=成功 I2C_STATUS_FAIL=失败
 */
uint8_t gd_i2c_read(uint8_t daddr, uint16_t regaddr, uint8_t* buff, uint16_t len, uint8_t addrlen)
{
    uint8_t i, addrbuff[2];
// uint8_t ret=I2C_STATUS_SUCCESS;
/* wait until I2C bus is idle */
#ifndef USE_I2C_OUTTIME_RETURN
    while (i2c_flag_get(I2C0, I2C_FLAG_I2CBSY))
        ;
#else
    if (gd_i2c_flag_check(I2C_FLAG_I2CBSY, SET) == I2C_STATUS_FAIL)
        return I2C_STATUS_FAIL;
#endif

    if (2 == len) {
        i2c_ackpos_config(I2C0, I2C_ACKPOS_NEXT);
    }

    /* send a start condition to I2C bus */
    i2c_start_on_bus(I2C0);

    /* wait until SBSEND bit is set */
#ifndef USE_I2C_OUTTIME_RETURN
    while (!i2c_flag_get(I2C0, I2C_FLAG_SBSEND))
        ;
#else
    if (gd_i2c_flag_check(I2C_FLAG_SBSEND, RESET) == I2C_STATUS_FAIL)
        return I2C_STATUS_FAIL;
#endif

    /* send slave address to I2C bus */
    i2c_master_addressing(I2C0, daddr, I2C_TRANSMITTER);

    /* wait until ADDSEND bit is set */
#ifndef USE_I2C_OUTTIME_RETURN
    while (!i2c_flag_get(I2C0, I2C_FLAG_ADDSEND))
        ;
#else
    if (gd_i2c_flag_check(I2C_FLAG_ADDSEND, RESET) == I2C_STATUS_FAIL)
        return I2C_STATUS_FAIL;
#endif
    /* clear the ADDSEND bit */
    i2c_flag_clear(I2C0, I2C_FLAG_ADDSEND);

    /* wait until the transmit data buffer is empty */
#ifndef USE_I2C_OUTTIME_RETURN
    while (SET != i2c_flag_get(I2C0, I2C_FLAG_TBE))
        ;
#else
    if (gd_i2c_flag_check(I2C_FLAG_TBE, RESET) == I2C_STATUS_FAIL)
        return I2C_STATUS_FAIL;
#endif
    /* enable I2C0*/
    i2c_enable(I2C0);

    if (addrlen > 1) {
        addrbuff[0] = regaddr >> 8;
        addrbuff[1] = regaddr;
    } else
        addrbuff[0] = regaddr;
    for (i = 0; i < addrlen; i++) {
        /* send the EEPROM's internal address to write to : only one byte address */
        i2c_data_transmit(I2C0, addrbuff[i]);

/* wait until BTC bit is set */
#ifndef USE_I2C_OUTTIME_RETURN
        while (!i2c_flag_get(I2C0, I2C_FLAG_BTC))
            ;
#else
        if (gd_i2c_flag_check(I2C_FLAG_BTC, RESET) == I2C_STATUS_FAIL)
            return I2C_STATUS_FAIL;
#endif
    }

    /* send a start condition to I2C bus */
    i2c_start_on_bus(I2C0);

    /* wait until SBSEND bit is set */
#ifndef USE_I2C_OUTTIME_RETURN
    while (!i2c_flag_get(I2C0, I2C_FLAG_SBSEND))
        ;
#else
    if (gd_i2c_flag_check(I2C_FLAG_SBSEND, RESET) == I2C_STATUS_FAIL)
        return I2C_STATUS_FAIL;
#endif
    /* send slave address to I2C bus */
    i2c_master_addressing(I2C0, daddr, I2C_RECEIVER);

    if (len < 3) {
        /* disable acknowledge */
        i2c_ack_config(I2C0, I2C_ACK_DISABLE);
    }

    /* wait until ADDSEND bit is set */
#ifndef USE_I2C_OUTTIME_RETURN
    while (!i2c_flag_get(I2C0, I2C_FLAG_ADDSEND))
        ;
#else
    if (gd_i2c_flag_check(I2C_FLAG_ADDSEND, RESET) == I2C_STATUS_FAIL)
        return I2C_STATUS_FAIL;
#endif
    /* clear the ADDSEND bit */
    i2c_flag_clear(I2C0, I2C_FLAG_ADDSEND);

    if (1 == len) {
        /* send a stop condition to I2C bus */
        i2c_stop_on_bus(I2C0);
    }

    /* while there is data to be read */
    while (len) {
        if (3 == len) {
            /* wait until BTC bit is set */
#ifndef USE_I2C_OUTTIME_RETURN
            while (!i2c_flag_get(I2C0, I2C_FLAG_BTC))
                ;
#else
            if (gd_i2c_flag_check(I2C_FLAG_BTC, RESET) == I2C_STATUS_FAIL)
                return I2C_STATUS_FAIL;
#endif
            /* disable acknowledge */
            i2c_ack_config(I2C0, I2C_ACK_DISABLE);
        }
        if (2 == len) {
            /* wait until BTC bit is set */
#ifndef USE_I2C_OUTTIME_RETURN
            while (!i2c_flag_get(I2C0, I2C_FLAG_BTC))
                ;
#else
            if (gd_i2c_flag_check(I2C_FLAG_BTC, RESET) == I2C_STATUS_FAIL)
                return I2C_STATUS_FAIL;
#endif
            /* send a stop condition to I2C bus */
            i2c_stop_on_bus(I2C0);
        }

        I2C_DELAYMS(1);
        /* wait until the RBNE bit is set and clear it */
        if (i2c_flag_get(I2C0, I2C_FLAG_RBNE)) {
            /* read a byte from the EEPROM */
            *buff = i2c_data_receive(I2C0);

            /* point to the next location where the byte read will be saved */
            buff++;

            /* decrement the read bytes counter */
            len--;
        }
    }

    /* wait until the stop condition is finished */
#ifndef USE_I2C_OUTTIME_RETURN
    while (I2C_CTL0(I2C0) & 0x0200)
        ;
#else
    if (gd_i2c_flag_check(I2C_CTL0(I2C0) & 0x0200, 0x0200) == I2C_STATUS_FAIL)
        return I2C_STATUS_FAIL;
#endif
    /* enable acknowledge */
    i2c_ack_config(I2C0, I2C_ACK_ENABLE);

    i2c_ackpos_config(I2C0, I2C_ACKPOS_CURRENT);

    return I2C_STATUS_SUCCESS;
}

/**
 * @brief I2C写入
 *
 * @param daddr 设备地址
 * @param regaddr 寄存器地址
 * @param buff 缓冲
 * @param len  长度
 * @param addrlen 寄存器地址长度
 * @return uint8_t I2C_STATUS_SUCCESS=成功 I2C_STATUS_FAIL=失败
 */
uint8_t gd_i2c_write(uint8_t daddr, uint16_t regaddr, uint8_t* buff, uint8_t len, uint8_t addrlen)
{
    // uint8_t ret=I2C_STATUS_SUCCESS;
    uint8_t i, addrbuff[2];

    /* wait until I2C bus is idle */
#ifndef USE_I2C_OUTTIME_RETURN
    while (i2c_flag_get(I2C0, I2C_FLAG_I2CBSY))
        ;
#else
    if (gd_i2c_flag_check(I2C_FLAG_I2CBSY, SET) == I2C_STATUS_FAIL)
        return I2C_STATUS_FAIL;
#endif
    /* send a start condition to I2C bus */
    i2c_start_on_bus(I2C0);

    /* wait until SBSEND bit is set */
#ifndef USE_I2C_OUTTIME_RETURN
    while (!i2c_flag_get(I2C0, I2C_FLAG_SBSEND))
        ;
#else
    if (gd_i2c_flag_check(I2C_FLAG_SBSEND, RESET) == I2C_STATUS_FAIL)
        return I2C_STATUS_FAIL;
#endif

    /* send slave address to I2C bus */
    i2c_master_addressing(I2C0, daddr, I2C_TRANSMITTER);

    /* wait until ADDSEND bit is set */
#ifndef USE_I2C_OUTTIME_RETURN
    while (!i2c_flag_get(I2C0, I2C_FLAG_ADDSEND))
        ;
#else
    if (gd_i2c_flag_check(I2C_FLAG_ADDSEND, RESET) == I2C_STATUS_FAIL)
        return I2C_STATUS_FAIL;
#endif
    /* clear the ADDSEND bit */
    i2c_flag_clear(I2C0, I2C_FLAG_ADDSEND);

    /* wait until the transmit data buffer is empty */
#ifndef USE_I2C_OUTTIME_RETURN
    while (SET != i2c_flag_get(I2C0, I2C_FLAG_TBE))
        ;
#else
    if (gd_i2c_flag_check(I2C_FLAG_TBE, RESET) == I2C_STATUS_FAIL)
        return I2C_STATUS_FAIL;
#endif
    if (addrlen > 1) {
        addrbuff[0] = regaddr >> 8;
        addrbuff[1] = regaddr;
    } else
        addrbuff[0] = regaddr;
    for (i = 0; i < addrlen; i++) {
        /* send the EEPROM's internal address to write to : only one byte address */
        i2c_data_transmit(I2C0, addrbuff[i]);

/* wait until BTC bit is set */
#ifndef USE_I2C_OUTTIME_RETURN
        while (!i2c_flag_get(I2C0, I2C_FLAG_BTC))
            ;
#else
        if (gd_i2c_flag_check(I2C_FLAG_BTC, RESET) == I2C_STATUS_FAIL)
            return I2C_STATUS_FAIL;
#endif
    }

    for (i = 0; i < len; i++) {
        /* send the byte to be written */
        i2c_data_transmit(I2C0, buff[i]);

        /* wait until BTC bit is set */
#ifndef USE_I2C_OUTTIME_RETURN
        while (!i2c_flag_get(I2C0, I2C_FLAG_TBE))
            ;
#else
        if (gd_i2c_flag_check(I2C_FLAG_TBE, RESET) == I2C_STATUS_FAIL)
            return I2C_STATUS_FAIL;
#endif
    }

    /* send a stop condition to I2C bus */
    i2c_stop_on_bus(I2C0);

    /* wait until the stop condition is finished */
#ifndef USE_I2C_OUTTIME_RETURN
    while (I2C_CTL0(I2C0) & 0x0200)
        ;
#else
    if (gd_i2c_flag_check(I2C_CTL0(I2C0) & 0x0200, 0x0200) == I2C_STATUS_FAIL)
        return I2C_STATUS_FAIL;
#endif
    return I2C_STATUS_SUCCESS;
}

/**
 * @brief I2C对CMD发送数据
 *
 * @param daddr 设备地址
 * @param cmd  命令字
 * @param data  数据
 * @return uint8_t I2C_STATUS_SUCCESS=成功 I2C_STATUS_FAIL=失败
 */
uint8_t gd_i2c_send_cmddata(uint8_t daddr, uint8_t cmd, uint8_t data)
{
    return gd_i2c_write(daddr, cmd, &data, 1, 1);
}
/**
 * @brief I2C从CMD读取数据(单字节)
 *
 * @param daddr 设备地址
 * @param cmd 命令字
 * @param data 数据
 * @return uint8_t 返回读取数据
 */
uint8_t gd_i2c_read_cmddata(uint8_t daddr, uint8_t cmd)
{
    uint8_t ret = 0;
    if (gd_i2c_read(daddr, cmd, &ret, 1, 1) == I2C_STATUS_FAIL) {
        ret = 0;
    }
    return ret;
}
/**
 * @brief I2C从CMD读取数据(2字节)
 *
 * @param daddr 设备地址
 * @param cmd 命令字
 * @param data 数据
 * @return uint8_t 返回读取数据
 */
uint16_t gd_i2c_read_cmdword(uint8_t daddr, uint8_t cmd)
{
    uint8_t ret[2] = { 0 };
    if (gd_i2c_read(daddr, cmd, ret, 2, 1) == I2C_STATUS_FAIL) {
        ret[0] = 0;
        ret[1] = 0;
    }
    return (((uint16_t)ret[1]) << 8) | ret[0];
}
/**
 * @brief I2C从CMD读取数据(N字节)
 *
 * @param daddr 设备地址
 * @param cmd 命令字
 * @param data 数据
 * @return uint8_t 0=成功 1=失败
 */
uint8_t gd_i2c_read_cmdspdata(uint8_t daddr, uint8_t cmd, uint8_t* buff, uint8_t len)
{
    uint8_t i, ret = 0;
    for (i = 0; i < len; i++) {
        if (gd_i2c_read(daddr, cmd + i, buff + i, 1, 1) == I2C_STATUS_FAIL) {
            memset(buff, 0, len);
            ret = 1;
            break;
        }
    }
    return ret;
}
