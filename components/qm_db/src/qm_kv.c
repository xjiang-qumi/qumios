
#include "qm_config.h"
#if CONFIG_QM_KV_SUPPORT

#if CONFIG_QM_KV_SYSTEM_SUPPORT
#include "qm_kv.h"
#include "qm_hal_kv.h"
#endif

#if CONFIG_QM_KV_DB_SUPPORT
#include "qm_kernel.h"
#include "flashdb.h"
#include "qm_log.h"
#include "qm_errno.h"
#include "fal_cfg.h"

static struct fdb_kvdb* qm_kvdb = NULL;
static qm_mutex_t kvdb_lock = {0};

static void lock(fdb_db_t db)
{
    qm_mutex_lock(&kvdb_lock, QM_WAIT_FOREVER);
}

static void unlock(fdb_db_t db)
{
    qm_mutex_unlock(&kvdb_lock);
}

int qm_kv_init(void)
{
    fdb_err_t ret = FDB_NO_ERR;
    if (qm_kvdb != NULL) {
        return QM_EOK;
    }
    qm_kvdb = qm_malloc(sizeof(struct fdb_kvdb));
    if (qm_kvdb == NULL) {
        return -QM_ENOMEM;
    }
    memset(qm_kvdb, 0, sizeof(struct fdb_kvdb));

        /* set the lock and unlock function if you want */
    fdb_kvdb_control(qm_kvdb, FDB_KVDB_CTRL_SET_LOCK, (void *)lock);
    fdb_kvdb_control(qm_kvdb, FDB_KVDB_CTRL_SET_UNLOCK, (void *)unlock);
        
    qm_mutex_new(&kvdb_lock);
    if(ret != QM_EOK){
        return ret;
    }

    ret = fdb_kvdb_init(qm_kvdb, "env", FAL_KV_FLASH_NAME, NULL, NULL);
    if(ret != FDB_NO_ERR){
        QM_LOGD(LOG_TAG, "fdb_kvdb_init ret %d", ret);
        qm_mutex_free(&kvdb_lock);
        return -QM_EIO;
    }
    return QM_EOK;
}

int qm_kv_set(const char *key, const void *value, int len, int sync)
{
    struct fdb_blob blob = {0};
    if (qm_kvdb == NULL) {
        QM_LOGE(LOG_TAG, "qm kv need init!!!");
        return -QM_EINIT;
    }
    if(key == NULL || value == NULL || len == 0){
        return -QM_EINVAL;
    }
	
	blob.buf = (void*)value;
	blob.size = len;
	int ret = fdb_kv_set_blob(qm_kvdb, key, &blob);
    if(ret != FDB_NO_ERR){
        QM_LOGE(LOG_TAG, "qm kv set fail!!!");
        return -QM_EIO;
    }
	return QM_EOK;
}

int qm_kv_get(const char *key, void *buffer, int *buffer_len)
{
    int ret = 0;
    struct fdb_blob blob = {0};

    if (qm_kvdb == NULL) {
        QM_LOGE(LOG_TAG, "qm kv need init!!!");
        return -QM_EINIT;
    }
    if(key == NULL || buffer == NULL || buffer_len == NULL){
        return -QM_EINVAL;
    }
	blob.buf = buffer;
	blob.size = *buffer_len;
    ret = (int)fdb_kv_get_blob(qm_kvdb, key, &blob);
    *buffer_len = ret;
    if(ret == 0){
        QM_LOGE(LOG_TAG, "qm kv get fail!!!");
        return -QM_EIO;
    }
	return QM_EOK;
}

int qm_kv_del(const char *key)
{
    if (qm_kvdb == NULL) {
        QM_LOGE(LOG_TAG, "qm kv need init!!!");
        return -QM_EINIT;
    }
    if(key == NULL){
        return -QM_EINVAL;
    }
	return fdb_kv_del(qm_kvdb, key);
}

int qm_kv_clean(void)
{
    fdb_err_t ret = FDB_NO_ERR;
    if (qm_kvdb == NULL) {
        QM_LOGE(LOG_TAG, "qm kv need init!!!");
        return -QM_EINIT;
    }
    ret = fdb_kv_set_default(qm_kvdb);
    if(ret != FDB_NO_ERR){
        return -QM_EIO;
    }
    return QM_EOK;
}

#endif

#if CONFIG_QM_KV_SYSTEM_SUPPORT == 1

int qm_kv_init(void)
{
    return qm_hal_kv_init();
}

int qm_kv_set(const char *key, const void *value, int len, int sync)
{
    return qm_hal_kv_set(key, value, len, sync);
}

int qm_kv_get(const char *key, void *buffer, int *buffer_len)
{
    return qm_hal_kv_get(key, buffer, buffer_len);
}

int qm_kv_del(const char *key)
{
    return qm_hal_kv_del(key);
}

int qm_kv_clean(void)
{
    return qm_hal_kv_clean();
}

#endif

#endif
