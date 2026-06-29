#include "qm_i2c.h"
#include "hal/i2c.h"
#include "qm_errno.h"

#define QM_I2C_REG_ADDR_WIDTH_0BIT   0
#define QM_I2C_REG_ADDR_WIDTH_8BIT   1
#define QM_I2C_REG_ADDR_WIDTH_16BIT  2

int32_t qm_i2c_init(qm_i2c_dev_t *i2c)
{
    I2C_MODE_E mode;

    if (i2c == NULL) {
        return -QM_EINVAL;
    }

    if (i2c->config.mode != QM_I2C_MODE_MASTER) {
        return -QM_EINVAL;
    }

    mode = (I2C_MODE_E)i2c->config.freq;

    if (hal_I2cInit(i2c->port, mode) != 0) {
        return -QM_EIO;
    }

    return QM_EOK;
}

int32_t qm_i2c_master_write(qm_i2c_dev_t *i2c, uint16_t dev_addr, const uint8_t *data,
                            uint16_t size, uint32_t timeout)
{
    (void)timeout;

    if (i2c == NULL || data == NULL || size == 0) {
        return -QM_EINVAL;
    }

    if (i2c->config.mode != QM_I2C_MODE_MASTER) {
        return -QM_EINVAL;
    }

    uint8_t reg_addr_width = QM_I2C_REG_ADDR_WIDTH_8BIT;
    if (i2c->priv != NULL) {
        reg_addr_width = *(uint8_t *)i2c->priv;
    }

    if (reg_addr_width == QM_I2C_REG_ADDR_WIDTH_16BIT) {
        if (size >= 2) {
            if (hal_I2cWrite(i2c->port, dev_addr, (data[0] << 8) | data[1], (uint8_t*)&data[2], size - 2) != 0) {
                return -QM_EIO;
            }
        } else {
            return -QM_EINVAL;
        }
    } else if (reg_addr_width == QM_I2C_REG_ADDR_WIDTH_0BIT) {
        if (hal_I2cWriteEx(i2c->port, dev_addr, NULL, 0, (uint8_t*)data, size) != 0) {
            return -QM_EIO;
        }
    } else {
        if (size >= 1) {
            if (hal_I2cWrite(i2c->port, dev_addr, data[0], (uint8_t*)&data[1], size - 1) != 0) {
                return -QM_EIO;
            }
        } else {
            return -QM_EINVAL;
        }
    }

    return QM_EOK;
}

int32_t qm_i2c_master_write_read(qm_i2c_dev_t *i2c, uint16_t dev_addr, 
                                const uint8_t* write_buffer, size_t write_size,
                                uint8_t* read_buffer, size_t read_size, uint32_t timeout)
{
    (void)timeout;

    if (i2c == NULL || write_buffer == NULL || read_buffer == NULL || 
        write_size == 0 || read_size == 0) {
        return -QM_EINVAL;
    }

    if (i2c->config.mode != QM_I2C_MODE_MASTER) {
        return -QM_EINVAL;
    }

    if (hal_I2cWrite(i2c->port, dev_addr, write_buffer[0], (uint8_t*)&write_buffer[1], write_size - 1) != 0) {
        return -QM_EIO;
    }

    if (hal_I2cRead(i2c->port, dev_addr, write_buffer[0], read_buffer, read_size) != 0) {
        return -QM_EIO;
    }

    return QM_EOK;
}

int32_t qm_i2c_slave_write(qm_i2c_dev_t *i2c, const uint8_t *data, uint16_t size, uint32_t timeout)
{
    (void)i2c;
    (void)data;
    (void)size;
    (void)timeout;

    return -QM_ERROR;
}

int32_t qm_i2c_slave_read(qm_i2c_dev_t *i2c, uint8_t *data, uint16_t size, uint32_t timeout)
{
    (void)i2c;
    (void)data;
    (void)size;
    (void)timeout;

    return -QM_ERROR;
}

int32_t qm_i2c_deinit(qm_i2c_dev_t *i2c)
{
    if (i2c == NULL) {
        return -QM_EINVAL;
    }

    return QM_EOK;
}