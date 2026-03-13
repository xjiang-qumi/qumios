#include "qm.h"
#include "qm_work.h"
#include "qm_eventqueue.h"

extern int32_t qm_queue_info_send(uint8_t queue_type, void *data, uint32_t len);
extern int32_t qm_queue_info_send_from_isr(uint8_t queue_type, void *data, uint32_t len);

typedef struct{
    uint8_t used;
    uint16_t event; 
    qm_event_cb cb;
    void *arg;
}qm_event_node_t;

typedef struct{
#if CONFIG_QM_OS_SUPPORT
    qm_mutex_t lock;
#endif
    qm_event_node_t event_node[CONFIG_QM_EVENT_NUM];
}qm_eventqueue_t;

static qm_eventqueue_t g_eventqueue = {0};

int32_t qm_eventqueue_init(void)
{

    qm_err_t ret = QM_EOK;
    memset(&g_eventqueue, 0, sizeof(qm_eventqueue_t));
#if CONFIG_QM_OS_SUPPORT
    ret = qm_mutex_new(&g_eventqueue.lock);
#endif
    return ret;
}

int32_t qm_event_handle(qm_input_event_t *input_event)
{
    int i = 0;
#if CONFIG_QM_OS_SUPPORT
    qm_mutex_lock(&g_eventqueue.lock, QM_WAIT_FOREVER);
#endif
    for(i = 0; i < CONFIG_QM_EVENT_NUM; i++){
        if(!g_eventqueue.event_node[i].used){
            continue;
        }
            
        if(g_eventqueue.event_node[i].event == input_event->event){
            if(input_event->arg && g_eventqueue.event_node[i].arg != input_event->arg){
                continue;
            }
            #if CONFIG_QM_OS_SUPPORT
                qm_mutex_unlock(&g_eventqueue.lock);
            #endif
                g_eventqueue.event_node[i].cb(input_event, g_eventqueue.event_node[i].arg);
            #if CONFIG_QM_OS_SUPPORT
                qm_mutex_lock(&g_eventqueue.lock, QM_WAIT_FOREVER);
            #endif
        }
    }    
     
#if CONFIG_QM_OS_SUPPORT
    qm_mutex_unlock(&g_eventqueue.lock);
#endif

    if(input_event->value && input_event->size){
        qm_free(input_event->value);
        input_event->value = NULL;
        input_event->size = 0;
    }
    return QM_EOK;
}

int32_t qm_event_register(uint16_t event, qm_event_cb cb, void *arg)
{
    int i = 0;
    qm_err_t ret = -QM_EFULL;
    if(cb == NULL){
        return -QM_EINVAL;
    }
#if CONFIG_QM_OS_SUPPORT
    qm_mutex_lock(&g_eventqueue.lock, QM_WAIT_FOREVER);
#endif
        //check
    for(i = 0; i < CONFIG_QM_EVENT_NUM; i++){
        if(g_eventqueue.event_node[i].used){
            if(g_eventqueue.event_node[i].event == event){
                ret = -QM_EINVAL;
                break;
            }
        }
    }
    for(i = 0; i < CONFIG_QM_EVENT_NUM; i++){
        if(g_eventqueue.event_node[i].used == 0){

            g_eventqueue.event_node[i].used = 1;
            g_eventqueue.event_node[i].arg = arg;
            g_eventqueue.event_node[i].cb = cb;
            g_eventqueue.event_node[i].event = event;
            ret = QM_EOK;
            break;
        }
    }
#if CONFIG_QM_OS_SUPPORT
    qm_mutex_unlock(&g_eventqueue.lock);
#endif

    return ret;
}

int32_t qm_event_unregister(uint16_t event, qm_event_cb cb, void *arg)
{
    uint16_t i = 0;
    if(cb == NULL){
        return -QM_EINVAL;
    }
#if CONFIG_QM_OS_SUPPORT
    qm_mutex_lock(&g_eventqueue.lock, QM_WAIT_FOREVER);
#endif
    for(i = 0; i < CONFIG_QM_EVENT_NUM; i++){
        if(g_eventqueue.event_node[i].used == 1){

            if((g_eventqueue.event_node[i].event == event) && 
            (g_eventqueue.event_node[i].cb == cb ) && 
            (g_eventqueue.event_node[i].arg == arg )){
                g_eventqueue.event_node[i].used = 0;
            #if CONFIG_QM_OS_SUPPORT
                qm_mutex_unlock(&g_eventqueue.lock);
            #endif
                return QM_EOK;
            }
        }
    }
#if CONFIG_QM_OS_SUPPORT
    qm_mutex_unlock(&g_eventqueue.lock);
#endif

    return QM_EOK;
}


static int32_t qm_input_event_set(qm_input_event_t *input_event, uint16_t event, uint16_t sub_event, void *value, uint32_t size, void *arg)
{
    input_event->time = qm_now_ms();
    input_event->event = event;
    input_event->sub_event = sub_event;
    if(value){
        if(size){
            input_event->value = (void*)qm_malloc(size);
            if(input_event->value == NULL){
                return -QM_ENOMEM;
            }
            memcpy(input_event->value, value, size);
        }else{
            input_event->value = value;
        }
    }
    input_event->arg = arg;
    input_event->size = size;
    return QM_EOK;
}

static int32_t qm_event_generic_post(uint16_t event, uint16_t sub_event, void *value, uint32_t size, void *arg)
{
    qm_err_t ret = QM_EOK;
    qm_input_event_t input_event = {0};

    ret = qm_input_event_set(&input_event, event, sub_event, value, size, arg);
    if(ret != QM_EOK){
        return ret;
    }
    return qm_queue_info_send(QM_QUEUE_EVENT_TYPE, (void*)&input_event, sizeof(qm_input_event_t));
}

static int32_t qm_event_generic_post_from_isr(uint16_t event, uint16_t sub_event, void *value, uint32_t size, void *arg)
{
    qm_err_t ret = QM_EOK;
    qm_input_event_t input_event = {0};

    ret = qm_input_event_set(&input_event, event, sub_event, value, size, arg);
    if(ret != QM_EOK){
        return ret;
    }
    return qm_queue_info_send_from_isr(QM_QUEUE_EVENT_TYPE, (void*)&input_event, sizeof(qm_input_event_t));
}

int32_t qm_event_post(uint16_t event, uint16_t sub_event, void *value, uint32_t size)
{
    return qm_event_generic_post(event, sub_event, value, size, NULL);
}

int32_t qm_event_post_from_isr(uint16_t event, uint16_t sub_event, void *value, uint32_t size)
{
    return qm_event_generic_post_from_isr(event, sub_event, value, size, NULL);
}

int32_t qm_event_ext_post(uint16_t event, uint16_t sub_event, void *value, uint32_t size, void *arg)
{
    if(arg == NULL){
        return -QM_EINVAL;
    }
    return qm_event_generic_post(event, sub_event, value, size, arg);
}

int32_t qm_event_ext_post_from_isr(uint16_t event, uint16_t sub_event, void *value, uint32_t size, void *arg)
{
    if(arg == NULL){
        return -QM_EINVAL;
    }
    return qm_event_generic_post_from_isr(event, sub_event, value, size, arg);
}

