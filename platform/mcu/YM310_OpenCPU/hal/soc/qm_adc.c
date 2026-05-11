#include "qm_adc.h"
#include "yopen_adc.h"

#define QM_ADC_DBG(...) //yopen_trace("[qm_adc][%s][%d]"__VA_ARGS__, __FUNCTION__, __LINE__)

typedef struct {
    yopen_adc_chan_id adc_channel;
    YOPEN_AdcAioResDiv_e resdiv;
    int last_value;
} qm_adc_priv_t;

int32_t qm_adc_init(qm_adc_dev_t *adc)
{
    qm_adc_priv_t *priv;

    if (adc == NULL) {
        return -1;
    }

    priv = (qm_adc_priv_t *)adc->priv;
    if (priv == NULL) {
        return -1;
    }

    switch (adc->port) {
        case 0:
            priv->adc_channel = YOPEN_ADC0_CHANNEL;
            break;
        case 1:
            priv->adc_channel = YOPEN_ADC1_CHANNEL;
            break;
        case 2:
            priv->adc_channel = YOPEN_ADC_TEM_CHANNEL;
            break;
        case 3:
            priv->adc_channel = YOPEN_ADC_VBAT_CHANNEL;
            break;
        default:
            return -1;
    }

    if (priv->resdiv == 0) {
        priv->resdiv = YOPEN_ADC_AIO_RESDIV_RATIO_1;
    }

    QM_ADC_DBG("init adc port %d channel %d resdiv %d", 
               adc->port, priv->adc_channel, priv->resdiv);
    return 0;
}

int32_t qm_adc_value_get(qm_adc_dev_t *adc, void *output, uint32_t timeout)
{
    yopen_errcode_adc_e ret;
    qm_adc_priv_t *priv;
    int adc_raw;

    if (adc == NULL || output == NULL) {
        return -1;
    }

    priv = (qm_adc_priv_t *)adc->priv;
    if (priv == NULL) {
        return -1;
    }

    QM_ADC_DBG("value_get adc port %d channel %d resdiv %d", 
               adc->port, priv->adc_channel, priv->resdiv);

    if (priv->adc_channel == YOPEN_ADC_VBAT_CHANNEL) {
        YOPEN_AdcVbatResdiv_e vbat_div = (YOPEN_AdcVbatResdiv_e)priv->resdiv;
        ret = yopen_adc_get_volt_raw(priv->adc_channel, (YOPEN_AdcAioResDiv_e)vbat_div, &adc_raw);
        if (ret != YOPEN_ADC_SUCCESS) {
            QM_ADC_DBG("yopen_adc_get_volt_raw failed %d", ret);
            return -1;
        }
        *(int *)output = adc_raw * 32 / (8 - vbat_div);
    } else {
        ret = yopen_adc_get_volt_raw(priv->adc_channel, priv->resdiv, &adc_raw);
        if (ret != YOPEN_ADC_SUCCESS) {
            QM_ADC_DBG("yopen_adc_get_volt_raw failed %d", ret);
            return -1;
        }
        switch (priv->resdiv) {
            case YOPEN_ADC_AIO_RESDIV_RATIO_1:
                *(int *)output = adc_raw;
                break;
            case YOPEN_ADC_AIO_RESDIV_RATIO_14OVER16:
                *(int *)output = adc_raw * 16 / 14;
                break;
            case YOPEN_ADC_AIO_RESDIV_RATIO_12OVER16:
                *(int *)output = adc_raw * 16 / 12;
                break;
            case YOPEN_ADC_AIO_RESDIV_RATIO_10OVER16:
                *(int *)output = adc_raw * 16 / 10;
                break;
            case YOPEN_ADC_AIO_RESDIV_RATIO_8OVER16:
                *(int *)output = adc_raw * 16 / 8;
                break;
            case YOPEN_ADC_AIO_RESDIV_RATIO_7OVER16:
                *(int *)output = adc_raw * 16 / 7;
                break;
            case YOPEN_ADC_AIO_RESDIV_RATIO_6OVER16:
                *(int *)output = adc_raw * 16 / 6;
                break;
            case YOPEN_ADC_AIO_RESDIV_RATIO_5OVER16:
                *(int *)output = adc_raw * 16 / 5;
                break;
            case YOPEN_ADC_AIO_RESDIV_RATIO_4OVER16:
                *(int *)output = adc_raw * 16 / 4;
                break;
            case YOPEN_ADC_AIO_RESDIV_RATIO_3OVER16:
                *(int *)output = adc_raw * 16 / 3;
                break;
            case YOPEN_ADC_AIO_RESDIV_RATIO_2OVER16:
                *(int *)output = adc_raw * 16 / 2;
                break;
            case YOPEN_ADC_AIO_RESDIV_RATIO_1OVER16:
                *(int *)output = adc_raw * 16 / 1;
                break;
            case YOPEN_ADC_AIO_RESDIV_BYPASS:
                *(int *)output = adc_raw;
                break;
            default:
                *(int *)output = adc_raw;
                break;
        }
    }

    priv->last_value = *(int *)output;
    QM_ADC_DBG("value_get result %d", *(int *)output);
    return 0;
}

int32_t qm_adc_start(qm_adc_dev_t *adc, void *data, uint32_t size)
{
    qm_adc_priv_t *priv;
    int i, ret;
    int *buffer = (int *)data;

    if (adc == NULL || data == NULL || size == 0) {
        return -1;
    }

    priv = (qm_adc_priv_t *)adc->priv;
    if (priv == NULL) {
        return -1;
    }

    QM_ADC_DBG("start adc port %d channel %d size %d", 
               adc->port, priv->adc_channel, size);

    for (i = 0; i < size / sizeof(int); i++) {
        ret = qm_adc_value_get(adc, &buffer[i], 0);
        if (ret != 0) {
            return -1;
        }
    }

    return 0;
}

int32_t qm_adc_stop(qm_adc_dev_t *adc)
{
    if (adc == NULL) {
        return -1;
    }

    QM_ADC_DBG("stop adc port %d", adc->port);
    return 0;
}

int32_t qm_adc_deinit(qm_adc_dev_t *adc)
{
    if (adc == NULL) {
        return -1;
    }

    QM_ADC_DBG("deinit adc port %d", adc->port);
    return 0;
}