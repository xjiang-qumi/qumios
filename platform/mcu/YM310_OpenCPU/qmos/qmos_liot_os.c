#include <qmos/qm.h>
#include "qm_config.h"
#include "qm_errno.h"
#include <stdio.h>
#include <string.h>
#include "stdlib.h"
#include "os_api.h"
#include "gbl_types.h"
#include "debug.h"

void qm_reboot(void)
{
    // TODO: 实现重启功能
}

const char *qm_version_get(void)
{
    return "V1.0.0";
}

int qm_task_new(qm_task_t *task, const char *name, void (*fn)(void *), void *arg, int stack_size, int prio)
{
    OSA_STATUS result = 0;
    OSTaskRef yopen_task = NULL; 

    result = OSATaskCreate(&yopen_task, NULL, (UINT32)stack_size, (UINT8)prio, (CHAR*)name, fn, arg);
    if(result == OS_SUCCESS){
        task->hdl = yopen_task;
        return QM_EOK; 
    }else{
        return -QM_ERROR;
    }
}

int qm_task_new_to_core(qm_task_t *task, const char *name, void (*fn)(void *), void *arg, int stack_size, int prio, int core_id)
{
    OSA_STATUS result = 0;
    OSTaskRef yopen_task = NULL; 

    result = OSATaskCreate(&yopen_task, NULL, (UINT32)stack_size, (UINT8)prio, (CHAR*)name, fn, arg);
    if(result == OS_SUCCESS){
        task->hdl = yopen_task;
        return QM_EOK; 
    }else{
        return -QM_ERROR;
    }
}

int qm_task_exit(qm_task_t *task)
{
    if(task == NULL){
        OSATaskDeleteExt(NULL);
    }else{
        OSATaskDeleteExt(task->hdl);
        task->hdl = NULL;
    }
    return QM_EOK;
}

int qm_mutex_new(qm_mutex_t *mutex)
{
    OSA_STATUS result = 0;
    OSMutexRef yopen_mutex = NULL;

    result = OSAMutexCreate(&yopen_mutex, OS_FIFO);
    if(result == OS_SUCCESS){
        mutex->hdl = yopen_mutex;
        return QM_EOK; 
    }else{
        return -QM_ERROR;
    }
}

int qm_mutex_free(qm_mutex_t *mutex)
{
    if (!mutex || !mutex->hdl) {
        return -QM_EINVAL;
    }    

    OSAMutexDelete(mutex->hdl);
    mutex->hdl = NULL;
    return QM_EOK;
}

int qm_mutex_lock(qm_mutex_t *mutex, unsigned int ms)
{
    OSA_STATUS result = 0;
    if (!mutex || !mutex->hdl) {
        return -QM_EINVAL;
    }
    
    if (ms == QM_WAIT_FOREVER) {
        result = OSAMutexLock(mutex->hdl, OS_SUSPEND);
    } else if (ms == 0) {
        result = OSAMutexLock(mutex->hdl, OS_NO_SUSPEND);
    } else {
        result = OSAMutexLock(mutex->hdl, MS2TICKS(ms));
    }
    
    return (result == OS_SUCCESS) ? QM_EOK : -QM_ERROR;
}

int qm_mutex_unlock(qm_mutex_t *mutex)
{
    OSA_STATUS result = 0;
    if (!mutex || !mutex->hdl) {
        return -QM_EINVAL;
    }
    result = OSAMutexUnlock(mutex->hdl);
    return (result == OS_SUCCESS) ? QM_EOK : -QM_ERROR;
}

int qm_mutex_is_valid(qm_mutex_t *mutex)
{
    return mutex && mutex->hdl != NULL;
}

int qm_sem_new(qm_sem_t *sem, int count)
{
    OSA_STATUS result = 0;
    OSSemaRef yopen_sem = NULL;

    result = OSASemaphoreCreate(&yopen_sem, (UINT32)count, OS_FIFO);
    if(result == OS_SUCCESS){
        sem->hdl = yopen_sem;
        return QM_EOK; 
    }else{
        return -QM_ERROR;
    }
}

int qm_sem_free(qm_sem_t *sem)
{
    if (sem == NULL || !sem->hdl) {
        return -QM_EINVAL;
    }
    OSASemaphoreDelete(sem->hdl);
    sem->hdl = NULL;
    return QM_EOK;
}

int qm_sem_wait(qm_sem_t *sem, unsigned int ms)
{
    OSA_STATUS result = 0;
    if (sem == NULL || !sem->hdl) {
        return -QM_EINVAL;
    }

    if (ms == QM_WAIT_FOREVER) {
        result = OSASemaphoreAcquire(sem->hdl, OS_SUSPEND);
    } else if (ms == 0) {
        result = OSASemaphoreAcquire(sem->hdl, OS_NO_SUSPEND);
    } else {
        result = OSASemaphoreAcquire(sem->hdl, MS2TICKS(ms));
    }
    
    return (result == OS_SUCCESS) ? QM_EOK : -QM_ERROR;
}

int qm_sem_signal(qm_sem_t *sem)
{
    OSA_STATUS result = 0;
    if (sem == NULL || !sem->hdl) {
        return -QM_EINVAL;
    }
    result = OSASemaphoreRelease(sem->hdl);
    return (result == OS_SUCCESS) ? QM_EOK : -QM_ERROR;
}

int qm_sem_signal_from_isr(qm_sem_t *sem)
{
    OSA_STATUS result = 0;
    if (sem == NULL || !sem->hdl) {
        return -QM_EINVAL;
    }
    result = OSASemaphoreRelease(sem->hdl);
    return (result == OS_SUCCESS) ? QM_EOK : -QM_ERROR;
}

int qm_sem_is_valid(qm_sem_t *sem)
{
    return sem && sem->hdl != NULL;
}

int qm_event_new(qm_event_t *event)
{
    OSA_STATUS result = 0;
    OSFlagRef yopen_flag = NULL;

    result = OSAFlagCreate(&yopen_flag);
    if(result == OS_SUCCESS){
        event->hdl = yopen_flag;
        return QM_EOK; 
    }else{
        return -QM_ERROR;
    }
}

int qm_event_free(qm_event_t *event)
{
    if (event == NULL || !event->hdl) {
        return -QM_EINVAL;
    }
    OSAFlagDelete(event->hdl);
    event->hdl = NULL;
    return QM_EOK;
}

int qm_event_get(qm_event_t *event, unsigned int wait_event_bits, unsigned char opt, unsigned int *actl_event_bits, unsigned int timeout)
{
    OSA_STATUS result = 0;
    UINT32 os_flags = 0;
    UINT32 os_operation = 0;
    
    if (event == NULL || !event->hdl || actl_event_bits == NULL) {
        return -QM_EINVAL;
    }

    switch (opt) {
        case QM_EVENT_OR:
            os_operation = OSA_FLAG_OR;
            break;
        case QM_EVENT_OR_CLEAR:
            os_operation = OSA_FLAG_OR_CLEAR;
            break;
        case QM_EVENT_AND:
            os_operation = OSA_FLAG_AND;
            break;
        case QM_EVENT_AND_CLEAR:
            os_operation = OSA_FLAG_AND_CLEAR;
            break;
        default:
            return -QM_EINVAL;
    }

    if (timeout == QM_WAIT_FOREVER) {
        result = OSAFlagWait(event->hdl, wait_event_bits, os_operation, &os_flags, OS_SUSPEND);
    } else if (timeout == 0) {
        result = OSAFlagWait(event->hdl, wait_event_bits, os_operation, &os_flags, OS_NO_SUSPEND);
    } else {
        result = OSAFlagWait(event->hdl, wait_event_bits, os_operation, &os_flags, MS2TICKS(timeout));
    }
    
    *actl_event_bits = os_flags;
    return (result == OS_SUCCESS) ? QM_EOK : -QM_ERROR;
}

int qm_event_set(qm_event_t *event, unsigned int event_bits)
{
    OSA_STATUS result = 0;
    if (event == NULL || !event->hdl) {
        return -QM_EINVAL;
    }
    result = OSAFlagSet(event->hdl, event_bits, OSA_FLAG_OR);
    return (result == OS_SUCCESS) ? QM_EOK : -QM_ERROR;
}

int qm_event_clear(qm_event_t *event, unsigned int event_bits)
{
    OSA_STATUS result = 0;
    if (event == NULL || !event->hdl) {
        return -QM_EINVAL;
    }
    result = OSAFlagSet(event->hdl, ~event_bits, OSA_FLAG_AND);
    return (result == OS_SUCCESS) ? QM_EOK : -QM_ERROR;
}

int qm_queue_new(qm_queue_t *queue, unsigned int queue_len, unsigned int queue_size)
{
    OSA_STATUS result = 0;
    OSMsgQRef yopen_queue = NULL;

    result = OSAMsgQCreate(&yopen_queue, NULL, queue_size, queue_len, OS_FIFO);
    if(result == OS_SUCCESS){
        queue->hdl = yopen_queue;
        return QM_EOK; 
    }else{
        return -QM_ERROR;
    }
}

int qm_queue_free(qm_queue_t *queue)
{
    if (queue == NULL || !queue->hdl) {
        return -QM_EINVAL;
    }
    OSAMsgQDelete(queue->hdl);
    queue->hdl = NULL;
    return QM_EOK;
}

int qm_queue_send(qm_queue_t *queue, void *msg, unsigned int size)
{
    OSA_STATUS result = 0;
    if (queue == NULL || !queue->hdl) {
        return -QM_EINVAL;
    }

    result = OSAMsgQSend(queue->hdl, size, (UINT8*)msg, OS_NO_SUSPEND);
    return (result == OS_SUCCESS) ? QM_EOK : -QM_ERROR;
}

int qm_queue_send_from_isr(qm_queue_t *queue, void *msg, unsigned int size)
{
    OSA_STATUS result = 0;
    if (queue == NULL || !queue->hdl) {
        return -QM_EINVAL;
    }

    result = OSAMsgQSend(queue->hdl, size, (UINT8*)msg, OS_NO_SUSPEND);
    return (result == OS_SUCCESS) ? QM_EOK : -QM_ERROR;
}

int qm_queue_recv(qm_queue_t *queue, void *msg, unsigned int *size, unsigned int ms)
{
    OSA_STATUS result = 0;
    if (queue == NULL || !queue->hdl || size == NULL) {
        return -QM_EINVAL;
    }

    if (ms == QM_WAIT_FOREVER) {
        result = OSAMsgQRecv(queue->hdl, (UINT8*)msg, *size, OS_SUSPEND);
    } else if (ms == 0) {
        result = OSAMsgQRecv(queue->hdl, (UINT8*)msg, *size, OS_NO_SUSPEND);
    } else {
        result = OSAMsgQRecv(queue->hdl, (UINT8*)msg, *size, MS2TICKS(ms));
    }
    
    return (result == OS_SUCCESS) ? QM_EOK : -QM_ERROR;
}

int qm_queue_is_valid(qm_queue_t *queue)
{
    return queue && queue->hdl != NULL;
}

#define QM_TIMER_MAX_NUM  20 //最大定时器数量

typedef struct 
{
    OSTimerRef timer;
    uint32_t timeout;
    qm_timer_t *qm_timer;
    qm_timer_cb_t cb;
    void *arg;
    int is_used;
} qm_timer_info_t;


static qm_timer_info_t qm_timer_info[QM_TIMER_MAX_NUM] = {0};
static qm_mutex_t g_timer_info_lock; 

static qm_timer_info_t *qm_timer_info_get(void)
{
    int i = 0;
    for(i = 0; i < QM_TIMER_MAX_NUM; i++){
        qm_mutex_lock(&g_timer_info_lock, QM_WAIT_FOREVER);
        if(!qm_timer_info[i].is_used){
            qm_timer_info[i].is_used = 1;
            qm_mutex_unlock(&g_timer_info_lock);
            return &qm_timer_info[i];
        }
        qm_mutex_unlock(&g_timer_info_lock);
    }
    return NULL;
}


static int qm_timer_info_reset(qm_timer_info_t *qm_timer_info)
{
    if(qm_timer_info == NULL) {
        return -QM_EINVAL;
    }
    qm_mutex_lock(&g_timer_info_lock, QM_WAIT_FOREVER);
    memset(qm_timer_info, 0, sizeof(qm_timer_info_t));
    qm_mutex_unlock(&g_timer_info_lock);
    return QM_EOK;
}

static void timer_callback(UINT32 timerArgc)
{
    qm_timer_info_t *qm_timer_info = (qm_timer_info_t*)timerArgc;
    if(qm_timer_info->cb){
        qm_timer_info->cb(qm_timer_info->qm_timer, qm_timer_info->arg);
    }
}

int qm_timer_new(qm_timer_t *timer, qm_timer_cb_t cb,
                  void *arg, int ms, int repeat)
{
    qm_timer_info_t *qm_timer_info = NULL;
    qm_err_t ret = QM_EOK;
    UINT32 rescheduleTime = 0;
    OSA_STATUS result = 0;
    qm_timer_info = qm_timer_info_get();
    if(qm_timer_info == NULL){
        return -QM_ERROR;
    }

    if(repeat){
        rescheduleTime = MS2TICKS(ms);
    }else{
        rescheduleTime = 0;
    }

    result = OSATimerCreate(&qm_timer_info->timer);
    if(result == OS_SUCCESS){
        ret = QM_EOK; 
    }else{
        qm_timer_info_reset(qm_timer_info);
        ret = -QM_ERROR;
        return ret;
    }

    timer->hdl = qm_timer_info;

    qm_timer_info->cb = cb;
    qm_timer_info->arg = arg;
    qm_timer_info->timeout = ms;
    qm_timer_info->qm_timer = timer;

    result = OSATimerStart(qm_timer_info->timer, MS2TICKS(ms), rescheduleTime, timer_callback, (UINT32)qm_timer_info);
    if(result != OS_SUCCESS){
        OSATimerDelete(qm_timer_info->timer);
        qm_timer_info_reset(qm_timer_info);
        ret = -QM_ERROR;
    }

    return ret;
}

int qm_timer_is_valid(qm_timer_t *timer)
{
    return timer && timer->hdl != NULL;
}

int qm_timer_free(qm_timer_t *timer)
{
    qm_timer_info_t *qm_timer_info = NULL;

    if(timer == NULL){
        return -QM_EINVAL;
    }

    qm_mutex_lock(&g_timer_info_lock, QM_WAIT_FOREVER);
    qm_timer_info = (qm_timer_info_t*)timer->hdl;
    qm_mutex_unlock(&g_timer_info_lock);

    if(qm_timer_info == NULL || qm_timer_info->timer == NULL) {
        return -QM_EINVAL;
    }
    
    OSATimerDelete(qm_timer_info->timer);
    qm_mutex_lock(&g_timer_info_lock, QM_WAIT_FOREVER);
    timer->hdl = NULL;
    qm_mutex_unlock(&g_timer_info_lock);
    qm_timer_info_reset(qm_timer_info);
    
    return QM_EOK;
}

int qm_timer_start(qm_timer_t *timer)
{
    OSA_STATUS result = 0;
    qm_timer_info_t *qm_timer_info = NULL;

    if(timer == NULL || timer->hdl == NULL){
        return -QM_EINVAL;
    }

    qm_mutex_lock(&g_timer_info_lock, QM_WAIT_FOREVER);
    qm_timer_info = (qm_timer_info_t*)timer->hdl;
    qm_mutex_unlock(&g_timer_info_lock);

    if(qm_timer_info == NULL) {
        return -QM_EINVAL;
    }

    result = OSATimerStart(qm_timer_info->timer, MS2TICKS(qm_timer_info->timeout), 
                          (qm_timer_info->timeout > 0) ? MS2TICKS(qm_timer_info->timeout) : 0,
                          timer_callback, (UINT32)qm_timer_info);
    return (result == OS_SUCCESS) ? QM_EOK : -QM_ERROR;
}

int qm_timer_start_from_isr(qm_timer_t *timer)
{
    return qm_timer_start(timer);
}

int qm_timer_stop(qm_timer_t *timer)
{
    OSA_STATUS result = 0;
    qm_timer_info_t *qm_timer_info = NULL;

    if(timer == NULL){
        return -QM_EINVAL;
    }

    qm_mutex_lock(&g_timer_info_lock, QM_WAIT_FOREVER);
    qm_timer_info = (qm_timer_info_t*)timer->hdl;
    qm_mutex_unlock(&g_timer_info_lock);
    
    if(qm_timer_info == NULL || qm_timer_info->timer == NULL) {
        return -QM_EINVAL;
    }

    result = OSATimerStop(qm_timer_info->timer);
    return (result == OS_SUCCESS) ? QM_EOK : -QM_ERROR;
}

int qm_timer_change(qm_timer_t *timer, int ms)
{
    qm_timer_info_t *qm_timer_info = NULL;
    OSA_STATUS result = 0;

    if(timer == NULL || timer->hdl == NULL){
        return -QM_EINVAL;
    }
    qm_timer_info = (qm_timer_info_t*)timer->hdl;

    OSATimerStop(qm_timer_info->timer);
    qm_timer_info->timeout = ms;
    result = OSATimerStart(qm_timer_info->timer, MS2TICKS(ms), 
                          (ms > 0) ? MS2TICKS(ms) : 0,
                          timer_callback, (UINT32)qm_timer_info);
    return (result == OS_SUCCESS) ? QM_EOK : -QM_ERROR;
}


#if !CONFIG_QM_MTRACE_SUPPORT

void* qm_calloc(unsigned int nitems, unsigned int size)
{
    return calloc(nitems, size);
}

void *qm_malloc(unsigned int size)
{
    return malloc(size);
}

void *qm_realloc(void *mem, unsigned int size)
{
    return realloc(mem, size);
}

void qm_free(void *mem)
{
    free(mem);
}

uint32_t qm_free_mem_get(void)
{
    return OsaGetDefaultMemPoolFreeSize();
}

#endif

uint32_t qm_now_ms(void)
{
    return OSAGetTicks() * OSA_TICK_FREQ_IN_MILLISEC;
}

int qm_msleep(int ms)
{
    OSATaskSleep(MS2TICKS(ms));
    return QM_EOK;
}

int qm_usleep(int us)
{
    DelayInMilliSecond(us);
    return QM_EOK;
}

#define LOG_MAXLEN 512

int qm_printf(const char * fmt, ...)
{
    static char l_pcOutBuf[LOG_MAXLEN + 128] = {0};
    va_list args;

    memset(l_pcOutBuf, 0, LOG_MAXLEN + 128);
    va_start(args, fmt);
    vsnprintf(l_pcOutBuf, LOG_MAXLEN, fmt, args);
    va_end(args);
    LOG_PRINTF("%s", l_pcOutBuf);
    return 0;
}


int qm_snprintf(char *str, const int len, const char *fmt, ...)
{
    va_list args;
    int     rc;
    va_start(args, fmt);
    rc = vsnprintf(str, len, fmt, args);
    va_end(args);

    return rc;
}

static uint32_t next = 0;  

/* RAND_MAX assumed to be 32767 */  
static uint32_t hal_rand(void) {  
    next = next * 1103515245 + 12345;  
    return((uint32_t)(next/65536) % 32768);  
}  

void qm_srandom(uint32_t seed)
{
    next = seed; 
}

uint32_t qm_random_get(uint32_t region)
{
    return (region > 0) ? (hal_rand() % region) : 0;
}

void qm_init(void)
{
    qm_mutex_new(&g_timer_info_lock);
}
