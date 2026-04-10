#include "qm_ota.h"
#include "qm_log.h"
#include "yopen_api_common.h"
#include "yopen_fota.h"

#define LOG_TAG "ota"

int qm_ota_start(void *arg)
{
    yopen_errcode_fota_e ret;

    ret = yopen_fotanvm_init();
    QM_LOGD(LOG_TAG, "yopen_fotanvm_init=%d", ret);
    if (ret == YOPEN_FOTA_UPGRADE_SUCCESS) {
        return QM_EOK;
    } else {
        return -QM_ERROR;
    }
}

int qm_ota_write(uint32_t *off_set, uint8_t *in_buf, uint32_t in_buf_len)
{
    uint32_t addr = *off_set;
    yopen_errcode_fota_e ret;
    ret = yopen_fotanvm_write(addr, in_buf, in_buf_len);
    if(ret != YOPEN_FOTA_UPGRADE_SUCCESS){
        return -QM_ERROR;
    }

    *off_set = addr + in_buf_len;
    
    return QM_EOK;
}

int qm_ota_read(uint32_t *off_set, uint8_t *out_buf, uint32_t out_buf_len)
{
    return QM_EOK;
}

int qm_ota_end(void *arg)
{
    yopen_errcode_fota_e ret;
    ret = yopen_fota_image_verify(0, YOPEN_FOTA_INTERNAL_FLASH);
    if(ret != YOPEN_FOTA_UPGRADE_SUCCESS){
        yopen_fota_clear(NULL, YOPEN_FOTA_INTERNAL_FLASH, 1);
        return -QM_ERROR;
    }else{
        qm_msleep(5000);
        yopen_fota_power_reset();
    }
    return QM_EOK;
}
