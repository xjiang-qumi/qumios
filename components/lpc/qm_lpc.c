#include "qm.h"
#include "qm_lpc.h"
#include "qm_lpc_hal.h"
#include "qm_utils_list.h"


#define LOG_TAG "qm_lpc"

typedef struct{
    uint32_t lpc_id;
    qm_lpc_mode_t mode;
    lpc_callback_t resume;
    lpc_callback_t prepare;
    qm_list_t   check_list;
#if CONFIG_QM_OS_SUPPORT
    qm_mutex_t  check_mutex;
#endif
}qm_lpc_ctx_t;

static qm_lpc_ctx_t g_lpc_ctx = {0};

int32_t qm_lpc_init(void)
{
#if CONFIG_QM_OS_SUPPORT
int32_t ret = QM_EOK;
#endif
    memset(&g_lpc_ctx, 0, sizeof(qm_lpc_ctx_t));
#if CONFIG_QM_OS_SUPPORT
    ret = qm_mutex_new(&g_lpc_ctx.check_mutex);
    if(ret != QM_EOK){
        goto _exit;
    }
#endif
    qm_hal_lpc_init();
    return QM_EOK;

#if CONFIG_QM_OS_SUPPORT
_exit:
    return ret;   
#endif
}


int32_t qm_lpc_wakeup_io_config(uint8_t pin, uint16_t type, lpc_callback_t wakeup_cb, bool_t enable)
{
    return qm_hal_lpc_wakeup_io_config(pin, type, wakeup_cb, enable);
}

int32_t qm_lpc_mode_set(qm_lpc_mode_t mode)
{
    int ret = QM_EOK;
    QM_RETURN_ON_FALSE(mode < QM_LPC_MAX, -QM_EINVAL, LOG_TAG, "mode error");
    ret = qm_hal_lpc_mode_set(mode);
    if(ret == QM_EOK){
        g_lpc_ctx.mode = mode;
    }
    return ret;
}

qm_lpc_mode_t qm_lpc_mode_get(void)
{
    return g_lpc_ctx.mode;
}


void *qm_lpc_check_register(lpc_callback_t callback)
{
    qm_list_node_t *node = NULL;
    QM_RETURN_ON_FALSE(callback, NULL, LOG_TAG, "callback NULL");

    node = qm_list_node_new((void *)callback);
    if(node == NULL){
        return NULL;
    }

#if CONFIG_QM_OS_SUPPORT
    qm_mutex_lock(&g_lpc_ctx.check_mutex, QM_WAIT_FOREVER);
#endif
    qm_list_rpush(&g_lpc_ctx.check_list, node);
#if CONFIG_QM_OS_SUPPORT
    qm_mutex_unlock(&g_lpc_ctx.check_mutex);
#endif
    return (void *)node;
}

int32_t qm_lpc_check_unregister(void *handle)
{
    QM_RETURN_ON_FALSE(handle, -QM_EINVAL, LOG_TAG, "handle NULL");
    
    qm_list_node_t *node = (qm_list_node_t *)handle;
#if CONFIG_QM_OS_SUPPORT
    qm_mutex_lock(&g_lpc_ctx.check_mutex, QM_WAIT_FOREVER);
#endif
    qm_list_remove(&g_lpc_ctx.check_list, node);
    qm_list_node_destroy(node);
#if CONFIG_QM_OS_SUPPORT
    qm_mutex_unlock(&g_lpc_ctx.check_mutex);
#endif
    return QM_EOK;
}


int32_t __attribute__((weak))  qm_hal_lpc_enable(void)
{
    return QM_EOK;
}

int32_t  __attribute__((weak)) qm_hal_lpc_disable(void)
{
    return QM_EOK;
}

int32_t qm_lpc_veto_add(qm_lpc_id_t id)
{
#if CONFIG_QM_OS_SUPPORT
    qm_mutex_lock(&g_lpc_ctx.check_mutex, QM_WAIT_FOREVER); 
#endif
    g_lpc_ctx.lpc_id |= id;
#if CONFIG_QM_OS_SUPPORT
    qm_mutex_unlock(&g_lpc_ctx.check_mutex);
#endif  

    if(g_lpc_ctx.lpc_id){
        qm_hal_lpc_disable();
    }

    return QM_EOK;
}

int32_t qm_lpc_veto_remove(qm_lpc_id_t id)
{
#if CONFIG_QM_OS_SUPPORT
    qm_mutex_lock(&g_lpc_ctx.check_mutex, QM_WAIT_FOREVER);
#endif
    g_lpc_ctx.lpc_id &= ~id;
#if CONFIG_QM_OS_SUPPORT
    qm_mutex_unlock(&g_lpc_ctx.check_mutex);
#endif  

    if(!g_lpc_ctx.lpc_id){
        qm_hal_lpc_enable();
    }

    return QM_EOK;
}

int32_t qm_lpc_check(void)  // If the return value is 0 means to disable sleep,others means enable.
{
    qm_list_iterator_t self;
    qm_list_node_t *node = NULL;
    lpc_callback_t check_cb = NULL;
    int check_ret = QM_LPC_DEEP_SLEEP;   

    self.direction = LIST_HEAD;
    self.next = g_lpc_ctx.check_list.head;

    if(g_lpc_ctx.lpc_id){
        return QM_LPC_NO_SLEEP;
    }

#if CONFIG_QM_OS_SUPPORT
    qm_mutex_lock(&g_lpc_ctx.check_mutex, QM_WAIT_FOREVER);
#endif
    node = qm_list_iterator_next(&self);
    
    while (node)
    {     
        if(node->val){
            check_cb  = (lpc_callback_t)node->val;
            check_ret = check_cb();
            if(check_ret == QM_LPC_NO_SLEEP){
                break;
            }
        }
        node = qm_list_iterator_next(&self);
    }
#if CONFIG_QM_OS_SUPPORT
    qm_mutex_unlock(&g_lpc_ctx.check_mutex);
#endif
    return check_ret;
}


int32_t qm_lpc_hw_register(lpc_callback_t prepare, lpc_callback_t resume)
{
    QM_RETURN_ON_FALSE(prepare, -QM_EINVAL, LOG_TAG, "prepare NULL");
    QM_RETURN_ON_FALSE(resume, -QM_EINVAL, LOG_TAG, "resume NULL");

    g_lpc_ctx.prepare = prepare;
    g_lpc_ctx.resume  = resume;

    return QM_EOK;

}


int32_t qm_lpc_prepare(void)
{
    QM_RETURN_ON_FALSE(g_lpc_ctx.prepare, -QM_EINVAL, LOG_TAG, "prepare NULL");
    return g_lpc_ctx.prepare();
}

int32_t qm_lpc_resume(void)
{
    QM_RETURN_ON_FALSE(g_lpc_ctx.resume, -QM_EINVAL, LOG_TAG, "resume NULL");
    return g_lpc_ctx.resume();
}


