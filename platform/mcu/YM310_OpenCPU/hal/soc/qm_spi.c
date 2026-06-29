#include "qm_spi.h"
#include "hal/spi.h"
#include "qm_errno.h"

int32_t qm_spi_init(qm_spi_dev_t *spi)
{
    SPI_MODE_E mode;

    if (spi == NULL) {
        return -QM_EINVAL;
    }

    switch (spi->config.clock_mode) {
        case QM_SPI_CLK_POL_LOW_PHA_1E:
            mode = SPI_MODE0;
            break;
        case QM_SPI_CLK_POL_LOW_PHA_2E:
            mode = SPI_MODE1;
            break;
        case QM_SPI_CLK_POL_HIGH_PHA_1E:
            mode = SPI_MODE2;
            break;
        case QM_SPI_CLK_POL_HIGH_PHA_2E:
            mode = SPI_MODE3;
            break;
        default:
            mode = SPI_MODE0;
            break;
    }

    if (hal_SpiInit(spi->port, mode, spi->config.freq, FALSE) != 0) {
        return -QM_EIO;
    }

    return QM_EOK;
}

int32_t qm_spi_write(qm_spi_dev_t *spi, const uint8_t *data, uint16_t size, uint32_t timeout)
{
    (void)timeout;

    if (spi == NULL || data == NULL || size == 0) {
        return -QM_EINVAL;
    }

    if (hal_SpiWrite(spi->port, (UINT8 *)data, size) != 0) {
        return -QM_EIO;
    }

    return size;
}

int32_t qm_spi_read(qm_spi_dev_t *spi, uint8_t *data, uint16_t size, uint32_t timeout)
{
    (void)timeout;

    if (spi == NULL || data == NULL || size == 0) {
        return -QM_EINVAL;
    }

    if (hal_SpiRead(spi->port, data, size) != 0) {
        return -QM_EIO;
    }

    return size;
}

int32_t qm_spi_write_read(qm_spi_dev_t *spi, const uint8_t *tx_data, uint16_t tx_size, uint8_t *rx_data, uint16_t rx_size, uint32_t timeout)
{
    (void)timeout;

    if (spi == NULL || tx_data == NULL || rx_data == NULL || tx_size == 0 || rx_size == 0) {
        return -QM_EINVAL;
    }

    uint16_t max_size = (tx_size > rx_size) ? tx_size : rx_size;

    if (hal_SpiWriteRead(spi->port, rx_data, (UINT8 *)tx_data, max_size) != 0) {
        return -QM_EIO;
    }

    return rx_size;
}

int32_t qm_spi_callback_reg(qm_spi_dev_t *spi, qm_spi_cb_t cb)
{
    (void)spi;
    (void)cb;

    return -QM_ERROR;
}

int32_t qm_spi_finalize(qm_spi_dev_t *spi)
{
    (void)spi;

    return QM_EOK;
}