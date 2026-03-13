#include "qm.h"
#include "qm_work.h"
#include "qm_workqueue.h"
#include "qm_eventqueue.h"

static qm_workqueue_t g_qm_workqueue = {0};
static void qm_work_handle(qm_workqueue_t *wq, qm_work_t *work);

extern int32_t qm_eventqueue_init(void);
extern int32_t qm_event_handle(qm_input_event_t *event);

int32_t qm_queue_info_generic_send(qm_queue_t *queue, uint8_t queue_type, void *data, uint32_t len)
{
    qm_err_t ret = QM_EOK;
    qm_queue_info_t queue_info = {0};

    queue_info.queue_type = queue_type;

    if(queue_type == QM_QUEUE_EVENT_TYPE){
        memcpy(&queue_info.info.event, data, len);
    }else if(queue_type == QM_QUEUE_WORK_TYPE){
        queue_info.info.work = (qm_work_t*)data;
    }else{
        return -QM_EINVAL;
    }

    ret = qm_queue_send(queue, (void*)&queue_info, sizeof(qm_queue_info_t));

    return ret;
}

int32_t qm_queue_info_generic_send_from_isr(qm_queue_t *queue, uint8_t queue_type, void *data, uint32_t len)
{
    qm_err_t ret = QM_EOK;
    qm_queue_info_t queue_info = {0};

    queue_info.queue_type = queue_type;

    if(queue_type == QM_QUEUE_EVENT_TYPE){
        memcpy(&queue_info.info.event, data, len);
    }else if(queue_type == QM_QUEUE_WORK_TYPE){
        queue_info.info.work = (qm_work_t*)data;
    }else{
        return -QM_EINVAL;
    }

    ret = qm_queue_send_from_isr(queue, (void*)&queue_info, sizeof(qm_queue_info_t));

    return ret;
}

int32_t qm_queue_info_send(uint8_t queue_type, void *data, uint32_t len)
{
    return qm_queue_info_generic_send(&g_qm_workqueue.queue, queue_type, data, len);
}

int32_t qm_queue_info_send_from_isr(uint8_t queue_type, void *data, uint32_t len)
{
    return qm_queue_info_generic_send_from_isr(&g_qm_workqueue.queue, queue_type, data, len);
}
#if CONFIG_QM_WORKQUEUE_NO_TASK
void qm_workqueue_process(void)
{
    qm_err_t ret = 0;
    qm_queue_info_t queue_info = {0};
    qm_workqueue_t *wq = &g_qm_workqueue;
    uint32_t size = 0;

    while (!qm_queue_is_empty(&wq->queue))
    {
        ret = qm_queue_recv(&wq->queue, &queue_info, &size, QM_NO_WAIT);
        if(ret != QM_EOK){
            return;
        }
        if(queue_info.queue_type == QM_QUEUE_EVENT_TYPE){
            qm_event_handle(&queue_info.info.event);
        }else if(queue_info.queue_type == QM_QUEUE_WORK_TYPE){
            qm_work_handle(wq, (qm_work_t*)queue_info.info.work);
        }
    }
}
#endif
#if CONFIG_QM_OS_SUPPORT
static void worker_task(void *arg)
{
    qm_err_t ret = 0;
    qm_queue_info_t queue_info = {0};
    qm_workqueue_t *wq = (qm_workqueue_t*)arg;
    unsigned int size = 0;
    while(1){
        ret = qm_queue_recv(&wq->queue, &queue_info, &size, QM_WAIT_FOREVER);
        if(ret != QM_EOK){
            continue;
        }
        if(queue_info.queue_type == QM_QUEUE_EVENT_TYPE){
            qm_event_handle(&queue_info.info.event);
        }else if(queue_info.queue_type == QM_QUEUE_WORK_TYPE){
            qm_work_handle(wq, (qm_work_t*)queue_info.info.work);
        }
    }
}
#else
static void worker_task(void *arg)
{
    qm_err_t ret = 0;
    unsigned int size = 0;
    qm_queue_info_t queue_info = {0};
   qm_workqueue_t *wq = &g_qm_workqueue;
    ret = qm_queue_recv(&wq->queue, &queue_info, &size, QM_WAIT_FOREVER);
    if(ret != QM_EOK){
        return;
    }
    if(queue_info.queue_type == QM_QUEUE_EVENT_TYPE){
        qm_event_handle(&queue_info.info.event);
    }else if(queue_info.queue_type == QM_QUEUE_WORK_TYPE){
        qm_work_handle(wq, (qm_work_t*)queue_info.info.work);
    }
}
#endif

static void qm_work_handle(qm_workqueue_t *wq, qm_work_t *work)
{
    if(work->dly > 0){
        work->dly = 0;
        qm_timer_stop(&work->timer);
        qm_timer_free(&work->timer);
    }
    /* do work */
    work->fn(work->arg);
}

int32_t qm_workqueue_create(qm_workqueue_t *workqueue, int pri, int stack_size)
{
    qm_err_t ret = 0;
    if(workqueue == NULL || stack_size == 0){
        return -QM_EINVAL;
    }

    ret = qm_queue_new(&workqueue->queue, CONFIG_QM_WORKQUEUE_NUM, sizeof(qm_queue_info_t));
    if(ret != QM_EOK){
        return ret;
    }

#if !CONFIG_QM_WORKQUEUE_NO_TASK
    ret = qm_task_new(&workqueue->worker, "worker", worker_task, (void*)workqueue, stack_size, pri);
    if(ret != QM_EOK){
        return ret;
    }
#endif 
    return QM_EOK;
}

static void work_timer_cb(qm_timer_t *timer, void *arg)
{
    qm_work_t *work = (qm_work_t*)arg;
    qm_workqueue_t *wq = (qm_workqueue_t*)work->wq;
    qm_queue_info_generic_send(&wq->queue, QM_QUEUE_WORK_TYPE, (void*)work, sizeof(void*));
}

int32_t qm_work_init(qm_work_t *work, void (*fn)(void *), void *arg, uint32_t dly)
{
    qm_err_t ret = 0;
    if(work == NULL || fn == NULL){
        return -QM_EINVAL;
    }
    work->fn  = fn;
    work->arg = arg;
    work->dly = dly;
    work->wq = NULL;

    if(work->dly > 0){
        ret = qm_timer_new(&work->timer, work_timer_cb, (void*)work, work->dly, 0);
        if(ret != QM_EOK){
            return ret;
        }
    }

    return QM_EOK;
}

int32_t qm_work_sched(qm_workqueue_t *workqueue, qm_work_t *work)
{
    qm_err_t ret = 0;

    if(workqueue == NULL || work == NULL){
        return -QM_EINVAL;
    }

    work->wq = (void*)workqueue;

    if(work->dly == 0){
        ret = qm_queue_info_generic_send(&workqueue->queue, QM_QUEUE_WORK_TYPE, (void*)work, sizeof(void*));
        if(ret != QM_EOK){
            return ret;
        }
    }else{
        ret = qm_timer_start(&work->timer);
        if(ret != QM_EOK){
            return ret;
        }
    }
    return QM_EOK;
}

int32_t qm_work_cancel(qm_work_t *work)
{
    if(work == NULL){
        return -QM_EINVAL;
    }
    if (work->dly > 0) {
        work->dly = 0;
        qm_timer_stop(&work->timer);
        qm_timer_free(&work->timer);
    }
    return QM_EOK;
}


int32_t qm_work_on_sched(qm_work_t *work)
{
    return qm_work_sched(&g_qm_workqueue, work);
}


int32_t qm_work_on_cancel(qm_work_t *work)
{
    return qm_work_cancel(work);
}

int32_t qm_work_on_init(void)
{
    qm_err_t ret = QM_EOK;
    ret = qm_eventqueue_init();
    if(ret != QM_EOK){
        return ret;
    }

    return qm_workqueue_create(&g_qm_workqueue, CONFIG_QM_WORKQUEUE_TASK_PRIO, CONFIG_QM_WORKQUEUE_TASK_SIZE);
}
