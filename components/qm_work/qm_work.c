#include "qm_work.h"
#include "qm_workqueue.h"
#include "qm_errno.h"


int32_t qm_post_delayed_action(qm_work_t *work, void (*fn)(void *), void *arg, uint32_t ms)
{
    qm_err_t ret = QM_EOK;

    if(work == NULL || fn == NULL){
        return -QM_EINVAL;
    }

    ret = qm_work_init(work, fn, arg, ms);
    if(ret != QM_EOK){
        goto __exit;
    }

    ret = qm_work_on_sched(work);
    if(ret != QM_EOK){
        goto __exit;
    }

__exit:
    return ret;
}

int32_t qm_cancel_delayed_action(qm_work_t *work)
{
    qm_err_t ret = QM_EOK;
    if(work == NULL){
        return -QM_EINVAL;
    }
    ret = qm_work_on_cancel(work);
    if(ret != QM_EOK){
        return ret;
    }
    return QM_EOK;
}
