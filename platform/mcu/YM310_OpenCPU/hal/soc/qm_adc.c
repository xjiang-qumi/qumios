#include "qm_adc.h"
#include "qm_errno.h"
#include "hal/adc.h"

#define QM_ADC_DBG(...) //QM_LOGD("[qm_adc][%s][%d]"__VA_ARGS__, __FUNCTION__, __LINE__)

int32_t qm_adc_init(qm_adc_dev_t *adc)
{
    if (adc == NULL) {
        return -QM_EINVAL;
    }

    QM_ADC_DBG("init adc port %d pin %d", adc->port, adc->pin.adc_pin);
    return QM_EOK;
}

int32_t qm_adc_value_get(qm_adc_dev_t *adc, void *output, uint32_t timeout)
{
    UINT16 value;

    (void)timeout;

    if (adc == NULL || output == NULL) {
        return -QM_EINVAL;
    }

    QM_ADC_DBG("value_get adc port %d pin %d", adc->port, adc->pin.adc_pin);

    if (adc->port == 3) {
        value = hal_AdcVbatRead();
    } else {
        value = hal_AdcRead(adc->pin.adc_pin);
    }

    *(int *)output = value;
    QM_ADC_DBG("value_get result %d", *(int *)output);
    return QM_EOK;
}

int32_t qm_adc_start(qm_adc_dev_t *adc, void *data, uint32_t size)
{
    int i, ret;
    int *buffer = (int *)data;

    if (adc == NULL || data == NULL || size == 0) {
        return -QM_EINVAL;
    }

    QM_ADC_DBG("start adc port %d pin %d size %d", adc->port, adc->pin.adc_pin, size);

    for (i = 0; i < size / sizeof(int); i++) {
        ret = qm_adc_value_get(adc, &buffer[i], 0);
        if (ret != QM_EOK) {
            return -QM_EIO;
        }
    }

    return QM_EOK;
}

int32_t qm_adc_stop(qm_adc_dev_t *adc)
{
    if (adc == NULL) {
        return -QM_EINVAL;
    }

    QM_ADC_DBG("stop adc port %d pin %d", adc->port, adc->pin.adc_pin);
    return QM_EOK;
}

int32_t qm_adc_deinit(qm_adc_dev_t *adc)
{
    if (adc == NULL) {
        return -QM_EINVAL;
    }

    QM_ADC_DBG("deinit adc port %d pin %d", adc->port, adc->pin.adc_pin);
    return QM_EOK;
}