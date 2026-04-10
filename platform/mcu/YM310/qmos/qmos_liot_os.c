#include <qmos/qm.h>
#include "qm_config.h"
#include "qm_errno.h"
#include <stdio.h>
#include <string.h>
#include "stdlib.h"
#include "debug_trace.h"
#include "FreeRTOSConfig.h"
#include "yopen_os.h"
#include "yopen_error.h"
#include "yopen_debug.h"

void qm_reboot(void)
{
    // yopen_power_reset(YOPEN_RESET_NORMAL);
    //TODO:
}

const char *qm_version_get(void)
{
    return "V1.0.0";
}

int qm_task_new(qm_task_t *task, const char *name, void (*fn)(void *), void *arg, int stack_size, int prio)
{
    YOpenOSStatus result = 0;
    yopen_task_t yopen_task = NULL; 

    result = yopen_rtos_task_create(&yopen_task, (uint32_t)stack_size, (uint8_t)prio, (char*)name, fn, arg);
    if(result == YOPEN_RET_OK){
        task->hdl = yopen_task;
        return QM_EOK; 
    }else{
        return -QM_ERROR;
    }
}

int qm_task_new_to_core(qm_task_t *task, const char *name, void (*fn)(void *), void *arg, int stack_size, int prio, int core_id)
{
    YOpenOSStatus result = 0;
    yopen_task_t yopen_task = NULL; 

    result = yopen_rtos_task_create(&yopen_task, (uint32_t)stack_size, (uint8_t)prio, (char*)name, fn, arg);
    if(result == YOPEN_RET_OK){
        task->hdl = yopen_task;
        return QM_EOK;
    }else{
        return -QM_ERROR;
    }
}

int qm_task_exit(qm_task_t *task)
{
    if(task == NULL){
        yopen_rtos_task_delete(NULL);
    }else{
        yopen_rtos_task_delete(task->hdl);
        task->hdl = NULL;
    }
    return QM_EOK;
}

int qm_mutex_new(qm_mutex_t *mutex)
{
    YOpenOSStatus result = 0;
    yopen_mutex_t yopen_mutex = NULL;

    result = yopen_rtos_mutex_create(&yopen_mutex);
    if(result == YOPEN_RET_OK){
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

    yopen_rtos_mutex_delete(mutex->hdl);
    mutex->hdl = NULL;
    return QM_EOK;
}

int qm_mutex_lock(qm_mutex_t *mutex, unsigned int ms)
{
    if (!mutex || !mutex->hdl) {
        return -QM_EINVAL;
    }
    yopen_rtos_mutex_lock(mutex->hdl, ms == QM_WAIT_FOREVER ? YOPEN_WAIT_FOREVER : ms);
    return QM_EOK;
}

int qm_mutex_unlock(qm_mutex_t *mutex)
{
    if (!mutex || !mutex->hdl) {
        return -QM_EINVAL;
    }
    yopen_rtos_mutex_unlock(mutex->hdl);
    return QM_EOK;
}

int qm_mutex_is_valid(qm_mutex_t *mutex)
{
    return mutex && mutex->hdl != NULL;
}

int qm_sem_new(qm_sem_t *sem, int count)
{
    YOpenOSStatus result = 0;
    yopen_sem_t yopen_sem = NULL;

    result = yopen_rtos_semaphore_create_ex(&yopen_sem, count, 100);
    if(result == YOPEN_RET_OK){
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
    yopen_rtos_semaphore_delete(sem->hdl);
    sem->hdl = NULL;
    return 0;
}

int qm_sem_wait(qm_sem_t *sem, unsigned int ms)
{
    YOpenOSStatus result = 0;
    if (sem == NULL || !sem->hdl) {
        return -QM_EINVAL;
    }

    result = yopen_rtos_semaphore_wait(sem->hdl, ms == QM_WAIT_FOREVER ? YOPEN_WAIT_FOREVER : ms);
    if(result == YOPEN_RET_OK){
        return QM_EOK; 
    }else{
        return -QM_ERROR;
    }
}

int qm_sem_signal(qm_sem_t *sem)
{
    if (sem == NULL || !sem->hdl) {
        return -QM_EINVAL;
    }
    yopen_rtos_semaphore_release(sem->hdl);
    return 0;
}

int qm_sem_signal_from_isr(qm_sem_t *sem)
{
    if (sem == NULL || !sem->hdl) {
        return -QM_EINVAL;
    }
    yopen_rtos_semaphore_release(sem->hdl);
    return 0;
}

int qm_sem_is_valid(qm_sem_t *sem)
{
    return sem && sem->hdl != NULL;
}

int qm_event_new(qm_event_t *event)
{
    return -QM_ERROR;
}

int qm_event_free(qm_event_t *event)
{
    return -QM_ERROR;
}

int qm_event_get(qm_event_t *event, unsigned int wait_event_bits, unsigned char opt, unsigned int *actl_event_bits, unsigned int timeout)
{
    return -QM_ERROR;
}

int qm_event_set(qm_event_t *event, unsigned int event_bits)
{
    return -QM_ERROR;
}

int qm_event_clear(qm_event_t *event, unsigned int event_bits)
{
    return -QM_ERROR;
}

int qm_queue_new(qm_queue_t *queue, unsigned int queue_len, unsigned int queue_size)
{
    YOpenOSStatus result = 0;
    yopen_queue_t yopen_queue = NULL;

    result = yopen_rtos_queue_create(&yopen_queue, queue_size, queue_len);
    if(result == YOPEN_RET_OK){
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
    yopen_rtos_queue_delete(queue->hdl);
    queue->hdl = NULL;
    return QM_EOK;
}

int qm_queue_send(qm_queue_t *queue, void *msg, unsigned int size)
{
    YOpenOSStatus result = 0;
    if (queue == NULL || !queue->hdl) {
        return -QM_EINVAL;
    }

    result = yopen_rtos_queue_release(queue->hdl, size, msg, YOPEN_NO_WAIT);
    if(result == YOPEN_RET_OK){
        return QM_EOK; 
    }else{
        return -QM_ERROR;
    }

}

int qm_queue_send_from_isr(qm_queue_t *queue, void *msg, unsigned int size)
{
    YOpenOSStatus result = 0;
    if (queue == NULL || !queue->hdl) {
        return -QM_EINVAL;
    }

    result = yopen_rtos_queue_release(queue->hdl, size, msg, YOPEN_NO_WAIT);
    if(result == YOPEN_RET_OK){
        return QM_EOK; 
    }else{
        return -QM_ERROR;
    }
}

int qm_queue_recv(qm_queue_t *queue, void *msg, unsigned int *size, unsigned int ms)
{
    YOpenOSStatus result = 0;
    if (queue == NULL || !queue->hdl || size == NULL) {
        return -QM_ERROR;
    }
    result = yopen_rtos_queue_wait(queue->hdl, msg, *size, ms == QM_WAIT_FOREVER ? YOPEN_WAIT_FOREVER : ms);
    if(result == YOPEN_RET_OK){
        return QM_EOK; 
    }else{
        return -QM_ERROR;
    }
}

int qm_queue_is_valid(qm_queue_t *queue)
{
    return queue && queue->hdl != NULL;
}

#define QM_TIMER_MAX_NUM  configTIMER_QUEUE_LENGTH

typedef struct 
{
    yopen_timer_t timer;
    uint32_t timeout;
    qm_timer_t *qm_timer;
    qm_timer_cb_t cb;
    void *arg;
    int is_used;
}qm_timer_info_t;


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

static void timer_callback(void *arg)
{
    qm_timer_info_t *qm_timer_info = (qm_timer_info_t*)arg;
    if(qm_timer_info->cb){
        qm_timer_info->cb(qm_timer_info->qm_timer, qm_timer_info->arg);
    }
}

int qm_timer_new(qm_timer_t *timer, qm_timer_cb_t cb,
                  void *arg, int ms, int repeat)
{
    qm_timer_info_t *qm_timer_info = NULL;
    qm_err_t ret = QM_EOK;
    yopen_timertype_t timertype = 0;
    YOpenOSStatus result = 0;
    qm_timer_info = qm_timer_info_get();
    if(qm_timer_info == NULL){
        return -QM_ERROR;
    }

    if(repeat){
        timertype = YOPEN_TimerPeriodic;
    }else{
        timertype = YOPEN_TimerOnce;
    }

    result = yopen_rtos_timer_create(&qm_timer_info->timer, timertype, timer_callback, (void*)qm_timer_info);
    if(result == YOPEN_RET_OK){
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
    
    yopen_rtos_timer_delete(qm_timer_info->timer);
    qm_mutex_lock(&g_timer_info_lock, QM_WAIT_FOREVER);
    timer->hdl = NULL;
    qm_mutex_unlock(&g_timer_info_lock);
    qm_timer_info_reset(qm_timer_info);
    
    return QM_EOK;
}

int qm_timer_start(qm_timer_t *timer)
{
    YOpenOSStatus result = 0;
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

    result = yopen_rtos_timer_start(qm_timer_info->timer, qm_timer_info->timeout);
    if(result == YOPEN_RET_OK){
        return QM_EOK; 
    }else{
        return -QM_ERROR;
    }
}

int qm_timer_start_from_isr(qm_timer_t *timer)
{
    YOpenOSStatus result = 0;
    qm_timer_info_t *qm_timer_info = NULL;

    if(timer == NULL || timer->hdl == NULL){
        return -QM_EINVAL;
    }
    qm_timer_info = (qm_timer_info_t*)timer->hdl;

    result = yopen_rtos_timer_start(qm_timer_info->timer, qm_timer_info->timeout);
    if(result == YOPEN_RET_OK){
        return QM_EOK; 
    }else{
        return -QM_ERROR;
    }
}

int qm_timer_stop(qm_timer_t *timer)
{
    YOpenOSStatus result = 0;
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

    result = yopen_rtos_timer_stop(qm_timer_info->timer);
    if(result == YOPEN_RET_OK){
        return QM_EOK; 
    }else{
        return -QM_ERROR;
    }
}

int qm_timer_change(qm_timer_t *timer, int ms)
{
    qm_timer_info_t *qm_timer_info = NULL;

    if(timer == NULL || timer->hdl == NULL){
        return -QM_EINVAL;
    }
    qm_timer_info = (qm_timer_info_t*)timer->hdl;

    yopen_rtos_timer_stop(qm_timer_info->timer);
    qm_timer_info->timeout = ms;
    yopen_rtos_timer_start(qm_timer_info->timer, ms);
    return QM_EOK;
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
    // yopen_os.h中没有直接的内存信息获取接口，返回0作为默认值
    return 0;
}

#endif

uint32_t qm_now_ms(void)
{
    return yopen_rtos_get_system_tick();
}

int qm_msleep(int ms)
{
    yopen_rtos_task_sleep_ms(ms);
    return QM_EOK;
}

int qm_usleep(int us)
{
    return -QM_ERROR;
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
    yopen_trace("%s", l_pcOutBuf);
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



