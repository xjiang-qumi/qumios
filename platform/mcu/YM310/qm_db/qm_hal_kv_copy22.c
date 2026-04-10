#include "qm.h"
#include "qm_hal_kv.h"

#define QM_KV_STORAGE_NAMESPACE "qm_kv"

int qm_hal_kv_init(void)
{
    return QM_EOK;
}

int qm_hal_kv_get(const char *key, void *buffer, int *buffer_len)
{
    return QM_EOK;
}

int qm_hal_kv_set(const char *key, const void *value, int len, int sync)
{
    return QM_EOK;
}

int qm_hal_kv_del(const char *key)
{
    return QM_EOK;
}

int qm_hal_kv_clean(void)
{
    return QM_EOK;
}
