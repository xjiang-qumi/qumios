#include "qm_i2c.h"
#include "yopen_i2c.h"
#include <string.h>
#include "yopen_gpio.h"

#define QM_I2C_REG_ADDR_WIDTH_0BIT   0   /**< No register address */
#define QM_I2C_REG_ADDR_WIDTH_8BIT   1   /**< 8-bit register address */
#define QM_I2C_REG_ADDR_WIDTH_16BIT  2   /**< 16-bit register address */

typedef struct {
    uint8_t sck_alt_func;
    uint8_t sda_alt_func;
} qm_i2c_priv_t;

int32_t qm_i2c_init(qm_i2c_dev_t *i2c)
{
    if (i2c == NULL) {
        return -1;
    }

    if (i2c->config.mode != QM_I2C_MODE_MASTER) {
        return -1;
    }

    yopen_i2c_mode_e mode;
    if (i2c->config.freq == QM_I2C_BUS_BIT_RATES_400K) {
        mode = FAST_MODE;
    } else {
        mode = STANDARD_MODE;
    }

    qm_i2c_priv_t *priv = (qm_i2c_priv_t *)i2c->priv;
    if (priv == NULL) {
        return -1;
    }

    // Set GPIO alternate function
    yopen_pin_set_func(i2c->pins.sck_pin, priv->sck_alt_func);
    yopen_pin_set_func(i2c->pins.sda_pin, priv->sda_alt_func);
    yopen_pin_set_pull(i2c->pins.sck_pin, FORCE_PULL_UP);
    yopen_pin_set_pull(i2c->pins.sda_pin, FORCE_PULL_UP);

    yopen_errcode_i2c_e ret = yopen_I2cInit((yopen_i2c_channel_e)i2c->port, mode);
    if (ret != YOPEN_I2C_SUCCESS) {
        return -1;
    }

    return 0;
}

int32_t qm_i2c_master_write(qm_i2c_dev_t *i2c, uint16_t dev_addr, const uint8_t *data,
                            uint16_t size, uint32_t timeout)
{
    (void)timeout;

    if (i2c == NULL || data == NULL || size == 0) {
        return -1;
    }

    if (i2c->config.mode != QM_I2C_MODE_MASTER) {
        return -1;
    }

    yopen_errcode_i2c_e ret;

    uint8_t reg_addr_width = QM_I2C_REG_ADDR_WIDTH_8BIT;
    if (i2c->priv != NULL) {
        reg_addr_width = *(uint8_t *)i2c->priv;
    }

    switch (reg_addr_width) {
        case QM_I2C_REG_ADDR_WIDTH_16BIT:
            if (size >= 2) {
                uint16_t addr = (data[0] << 8) | data[1];
                ret = yopen_I2cWrite_16bit_addr((yopen_i2c_channel_e)i2c->port, (uint8_t)dev_addr,
                                                addr, (uint8_t*)&data[2], size - 2);
            } else {
                return -1;
            }
            break;
        case QM_I2C_REG_ADDR_WIDTH_0BIT:
            ret = yopen_I2cWrite_Noaddr((yopen_i2c_channel_e)i2c->port, (uint8_t)dev_addr,
                                        (uint8_t*)data, size);
            break;
        case QM_I2C_REG_ADDR_WIDTH_8BIT:
        default:
            if (size >= 1) {
                uint8_t addr = data[0];
                ret = yopen_I2cWrite((yopen_i2c_channel_e)i2c->port, (uint8_t)dev_addr,
                                    addr, (uint8_t*)&data[1], size - 1);
            } else {
                return -1;
            }
            break;
    }

    if (ret != YOPEN_I2C_SUCCESS) {
        return -1;
    }

    return 0;
}

int32_t qm_i2c_master_write_read(qm_i2c_dev_t *i2c, uint16_t dev_addr, 
                                const uint8_t* write_buffer, size_t write_size,
                                uint8_t* read_buffer, size_t read_size, uint32_t timeout)
{
    (void)timeout;

    if (i2c == NULL || write_buffer == NULL || read_buffer == NULL || 
        write_size == 0 || read_size == 0) {
        return -1;
    }

    if (i2c->config.mode != QM_I2C_MODE_MASTER) {
        return -1;
    }

    yopen_errcode_i2c_e ret;

    uint8_t reg_addr_width = QM_I2C_REG_ADDR_WIDTH_8BIT;
    if (i2c->priv != NULL) {
        reg_addr_width = *(uint8_t *)i2c->priv;
    }

    switch (reg_addr_width) {
        case QM_I2C_REG_ADDR_WIDTH_16BIT:
            if (write_size >= 2) {
                uint16_t addr = (write_buffer[0] << 8) | write_buffer[1];
                ret = yopen_I2cWrite_16bit_addr((yopen_i2c_channel_e)i2c->port, (uint8_t)dev_addr,
                                                addr, (uint8_t*)&write_buffer[2], write_size - 2);
                if (ret != YOPEN_I2C_SUCCESS) {
                    return -1;
                }
                ret = yopen_I2cRead_Noaddr((yopen_i2c_channel_e)i2c->port, (uint8_t)dev_addr,
                                            read_buffer, read_size);
            } else {
                return -1;
            }
            break;
        case QM_I2C_REG_ADDR_WIDTH_0BIT:
            ret = yopen_I2cWrite_Noaddr((yopen_i2c_channel_e)i2c->port, (uint8_t)dev_addr,
                                        (uint8_t*)write_buffer, write_size);
            if (ret != YOPEN_I2C_SUCCESS) {
                return -1;
            }
            ret = yopen_I2cRead_Noaddr((yopen_i2c_channel_e)i2c->port, (uint8_t)dev_addr,
                                        read_buffer, read_size);
            break;
        case QM_I2C_REG_ADDR_WIDTH_8BIT:
        default:
            if (write_size >= 1) {
                uint8_t addr = write_buffer[0];
                ret = yopen_I2cWrite((yopen_i2c_channel_e)i2c->port, (uint8_t)dev_addr,
                                    addr, (uint8_t*)&write_buffer[1], write_size - 1);
                if (ret != YOPEN_I2C_SUCCESS) {
                    return -1;
                }
                ret = yopen_I2cRead_Noaddr((yopen_i2c_channel_e)i2c->port, (uint8_t)dev_addr,
                                            read_buffer, read_size);
            } else {
                return -1;
            }
            break;
    }

    if (ret != YOPEN_I2C_SUCCESS) {
        return -1;
    }

    return 0;
}

int32_t qm_i2c_slave_write(qm_i2c_dev_t *i2c, const uint8_t *data, uint16_t size, uint32_t timeout)
{
    (void)i2c;
    (void)data;
    (void)size;
    (void)timeout;

    return -1;
}

int32_t qm_i2c_slave_read(qm_i2c_dev_t *i2c, uint8_t *data, uint16_t size, uint32_t timeout)
{
    (void)i2c;
    (void)data;
    (void)size;
    (void)timeout;

    return -1;
}

int32_t qm_i2c_deinit(qm_i2c_dev_t *i2c)
{
    if (i2c == NULL) {
        return -1;
    }

    yopen_errcode_i2c_e ret = yopen_I2cRelease((yopen_i2c_channel_e)i2c->port);
    if (ret != YOPEN_I2C_SUCCESS) {
        return -1;
    }

    return 0;
}