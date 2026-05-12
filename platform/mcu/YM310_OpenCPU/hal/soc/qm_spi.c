#include <string.h>
#include "qm_spi.h"
#include "yopen_spi.h"

typedef struct {
    uint8_t cs_alt_func;
    uint8_t sck_alt_func;
    uint8_t mosi_alt_func;
    uint8_t miso_alt_func;
} qm_spi_priv_t;

int32_t qm_spi_init(qm_spi_dev_t *spi)
{
    if (spi == NULL) {
        return -1;
    }

    yopen_spi_config_s config = {0};
    config.port = (yopen_spi_port_e)spi->port;
    config.framesize = spi->config.data_width;

    qm_spi_priv_t *priv = (qm_spi_priv_t *)spi->priv;
    yopen_pin_set_func(spi->pins.cs_pin, priv->cs_alt_func);
    yopen_pin_set_func(spi->pins.sck_pin, priv->sck_alt_func);
    yopen_pin_set_func(spi->pins.mosi_pin, priv->mosi_alt_func);
    yopen_pin_set_func(spi->pins.miso_pin, priv->miso_alt_func);

    switch (spi->config.freq) {
        case 812500:
            config.spiclk = YOPEN_SPI_CLK_812_5KHZ;
            break;
        case 1625000:
            config.spiclk = YOPEN_SPI_CLK_1_625MHZ;
            break;
        case 3250000:
            config.spiclk = YOPEN_SPI_CLK_3_25MHZ;
            break;
        case 6500000:
            config.spiclk = YOPEN_SPI_CLK_6_5MHZ;
            break;
        case 13000000:
            config.spiclk = YOPEN_SPI_CLK_13MHZ;
            break;
        default:
            config.spiclk = YOPEN_SPI_CLK_13MHZ;
            break;
    }

    switch (spi->config.clock_mode) {
        case QM_SPI_CLK_POL_LOW_PHA_1E:
            config.cpol = YOPEN_SPI_CPOL_LOW;
            config.cpha = YOPEN_SPI_CPHA_1Edge;
            break;
        case QM_SPI_CLK_POL_LOW_PHA_2E:
            config.cpol = YOPEN_SPI_CPOL_LOW;
            config.cpha = YOPEN_SPI_CPHA_2Edge;
            break;
        case QM_SPI_CLK_POL_HIGH_PHA_1E:
            config.cpol = YOPEN_SPI_CPOL_HIGH;
            config.cpha = YOPEN_SPI_CPHA_1Edge;
            break;
        case QM_SPI_CLK_POL_HIGH_PHA_2E:
            config.cpol = YOPEN_SPI_CPOL_HIGH;
            config.cpha = YOPEN_SPI_CPHA_2Edge;
            break;
        default:
            config.cpol = YOPEN_SPI_CPOL_LOW;
            config.cpha = YOPEN_SPI_CPHA_1Edge;
            break;
    }

    if (spi->config.mode == QM_SPI_MODE_MASTER) {
        config.transmode = YOPEN_SPI_DIRECT_POLLING;
    } else {
        config.transmode = YOPEN_SPI_SLAVE_DIRECT_POLLING;
    }

    yopen_errcode_spi_e ret = yopen_spi_init(config);
    if (ret != YOPEN_SPI_SUCCESS) {
        return -1;
    }

    return 0;
}

int32_t qm_spi_write(qm_spi_dev_t *spi, const uint8_t *data, uint16_t size, uint32_t timeout)
{
    (void)timeout;

    if (spi == NULL || data == NULL || size == 0) {
        return -1;
    }

    yopen_errcode_spi_e ret = yopen_spi_write((yopen_spi_port_e)spi->port, (unsigned char *)data, size);
    if (ret != YOPEN_SPI_SUCCESS) {
        return -1;
    }

    return size;
}

int32_t qm_spi_read(qm_spi_dev_t *spi, uint8_t *data, uint16_t size, uint32_t timeout)
{
    (void)timeout;

    if (spi == NULL || data == NULL || size == 0) {
        return -1;
    }

    yopen_errcode_spi_e ret = yopen_spi_read((yopen_spi_port_e)spi->port, (unsigned char *)data, size);
    if (ret != YOPEN_SPI_SUCCESS) {
        return -1;
    }

    return size;
}

int32_t qm_spi_write_read(qm_spi_dev_t *spi, const uint8_t *tx_data, uint16_t tx_size, uint8_t *rx_data, uint16_t rx_size, uint32_t timeout)
{
    (void)timeout;

    if (spi == NULL || tx_data == NULL || rx_data == NULL || tx_size == 0 || rx_size == 0) {
        return -1;
    }

    uint16_t max_size = (tx_size > rx_size) ? tx_size : rx_size;
    
    unsigned char *tx_buf = (unsigned char *)tx_data;
    unsigned char *rx_buf = (unsigned char *)rx_data;

    if (tx_size < rx_size) {
        tx_buf = (unsigned char *)malloc(rx_size);
        if (tx_buf == NULL) {
            return -1;
        }
        memcpy(tx_buf, tx_data, tx_size);
        memset(tx_buf + tx_size, 0, rx_size - tx_size);
    } else if (rx_size < tx_size) {
        rx_buf = (unsigned char *)malloc(tx_size);
        if (rx_buf == NULL) {
            if (tx_size < rx_size) free(tx_buf);
            return -1;
        }
    }

    yopen_errcode_spi_e ret = yopen_spi_write_read((yopen_spi_port_e)spi->port, rx_buf, tx_buf, max_size);
    
    if (rx_buf != rx_data) {
        memcpy(rx_data, rx_buf, rx_size);
        free(rx_buf);
    }
    if (tx_buf != tx_data) {
        free(tx_buf);
    }

    if (ret != YOPEN_SPI_SUCCESS) {
        return -1;
    }

    return rx_size;
}

int32_t qm_spi_callback_reg(qm_spi_dev_t *spi, qm_spi_cb_t cb)
{
    if (spi == NULL || cb == NULL) {
        return -1;
    }

    yopen_errcode_spi_e ret = yopen_spi_callback_set((yopen_spi_port_e)spi->port, (yopen_spi_callback)cb);
    if (ret != YOPEN_SPI_SUCCESS) {
        return -1;
    }

    return 0;
}

int32_t qm_spi_finalize(qm_spi_dev_t *spi)
{
    if (spi == NULL) {
        return -1;
    }

    yopen_errcode_spi_e ret = yopen_spi_release((yopen_spi_port_e)spi->port);
    if (ret != YOPEN_SPI_SUCCESS) {
        return -1;
    }

    return 0;
}