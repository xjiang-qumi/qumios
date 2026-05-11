#include "qm_ota.h"
#include "qm_log.h"

#define LOG_TAG "ota"

int qm_ota_start(void *arg)
{
    return QM_EOK;
}

int qm_ota_write(uint32_t *off_set, uint8_t *in_buf, uint32_t in_buf_len)
{
    return QM_EOK;
}

int qm_ota_read(uint32_t *off_set, uint8_t *out_buf, uint32_t out_buf_len)
{
    return QM_EOK;
}

int qm_ota_end(void *arg)
{
    return QM_EOK;
}
