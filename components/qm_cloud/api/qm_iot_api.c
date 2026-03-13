#include "qm.h"
#include "qm_iot_api.h"
#include "qm_iot_core.h"
#include "qm_iot_config.h"

#define LOG_TAG "IOT_API"


typedef struct 
{
    void *core_handle;
    void *user_data;

    qm_iot_event_handler_t event_handler;
    qm_iot_recv_handler_t recv_handler;
}qm_iot_api_handle_t;

int32_t qm_iot_start(void *handle)
{
    qm_iot_api_handle_t *api_handle = (qm_iot_api_handle_t *)handle;
    if(api_handle == NULL){
        return -QM_EINVAL;
    }
    
    return qm_iot_core_start(api_handle->core_handle);
}

void *qm_iot_init(void)
{
    qm_iot_api_handle_t *api_handle = (qm_iot_api_handle_t *)qm_malloc(sizeof(qm_iot_api_handle_t));
    if(api_handle == NULL){
        return NULL;
    }
    memset(api_handle, 0, sizeof(qm_iot_api_handle_t));

    api_handle->core_handle = qm_iot_core_init(api_handle);
    if(api_handle->core_handle == NULL) {
        qm_free(api_handle);
        return NULL;
    }
    
    return api_handle;
}

int32_t qm_iot_setopt(void *handle, qm_iot_option_t option, void *data)
{
    int ret = QM_EOK;
    qm_iot_api_handle_t *api_handle = (qm_iot_api_handle_t *)handle;
    if(api_handle == NULL || data == NULL){
        return -QM_EINVAL;
    }

    switch (option)
    {
        case QM_IOT_OPT_VERSION:
            ret = qm_iot_version_set(*((uint32_t *)data));
        break;

        case QM_IOT_OPT_PRODUCT_ID:
            ret = qm_iot_pid_set(*((uint32_t *)data));
        break;

        case QM_IOT_OPT_PRODUCT_SECRET:
            ret = qm_iot_prodtuct_sercet_set((char *)data, strlen((char *)data));
        break;

        case QM_IOT_OPT_RECV_HANDLER: 
            api_handle->recv_handler = (qm_iot_recv_handler_t)data;
        break;

        case QM_IOT_OPT_EVENT_HANDLER:
            api_handle->event_handler = (qm_iot_event_handler_t)data;
        break;

        case QM_IOT_OPT_USERDATA:
            api_handle->user_data = (void *)data;
        break;

        default:
            ret = -QM_EINVAL;
        break;
    }

    return ret;
}

int32_t qm_iot_getopt(void *handle, qm_iot_option_t option, void **data)
{
    qm_iot_api_handle_t *api_handle = (qm_iot_api_handle_t *)handle;
    if(api_handle == NULL || data == NULL){
        return -QM_EINVAL;
    }

    switch (option)
    {
        case QM_IOT_OPT_RECV_HANDLER: 
            *((qm_iot_recv_handler_t *)data) = api_handle->recv_handler;
        break;

        case QM_IOT_OPT_EVENT_HANDLER:
        {
            *((qm_iot_event_handler_t *)data) = api_handle->event_handler;
        }
        break;

        case QM_IOT_OPT_USERDATA:
            *data = api_handle->user_data;
        break;

        default:

        break;
    }

    return QM_EOK;
}


int32_t qm_iot_set_weahter_request(void *handle)
{
    uint32_t main_did = qm_iot_did_get();
    qm_iot_api_handle_t *api_handle = (qm_iot_api_handle_t *)handle;
    if(api_handle == NULL || !main_did){
        return -QM_EINVAL;
    }
    return qm_iot_core_send(api_handle->core_handle, main_did, QM_IOT_CORE_MSG_TYPE_WEATHER_REQUEST, NULL);
}


int32_t qm_iot_property_get_rsponse(void *handle, qm_spec_property_operation_t *property_operation)
{
    uint32_t main_did = qm_iot_did_get();
    qm_iot_api_handle_t *api_handle = (qm_iot_api_handle_t *)handle;
    
    if(!main_did || api_handle == NULL || property_operation == NULL){
        return -QM_EINVAL;
    }
    return qm_iot_core_send(api_handle->core_handle, main_did, QM_IOT_CORE_MSG_TYPE_GET_RSP, property_operation);
}


int32_t qm_iot_property_get_request(void *handle, qm_spec_property_operation_t *property_operation)
{
    uint32_t main_did = qm_iot_did_get();
    qm_iot_api_handle_t *api_handle = (qm_iot_api_handle_t *)handle;
    
    if(!main_did || api_handle == NULL || property_operation == NULL){
        return -QM_EINVAL;
    }
    return qm_iot_core_send(api_handle->core_handle, main_did, QM_IOT_CORE_MSG_TYPE_GET_REQ, property_operation);
}

int32_t qm_iot_property_report(void *handle, qm_spec_property_operation_t *property_operation)
{
    uint32_t main_did = qm_iot_did_get();
    qm_iot_api_handle_t *api_handle = (qm_iot_api_handle_t *)handle;
    
    if(!main_did || api_handle == NULL || property_operation == NULL){
        return -QM_EINVAL;
    }
    return qm_iot_core_send(api_handle->core_handle, main_did, QM_IOT_CORE_MSG_TYPE_REPORT, property_operation);
}


int32_t qm_iot_report(void *handle, qm_spec_property_operation_t *property_operation)
{
    uint32_t main_did = qm_iot_did_get();
    qm_iot_api_handle_t *api_handle = (qm_iot_api_handle_t *)handle;
    
    if(!main_did || api_handle == NULL || property_operation == NULL){
        return -QM_EINVAL;
    }
    return qm_iot_core_send(api_handle->core_handle, main_did, QM_IOT_CORE_MSG_TYPE_AWS_REPORT, property_operation);
}

int32_t qm_iot_reset(void *handle)
{
    qm_iot_api_handle_t *api_handle = (qm_iot_api_handle_t *)handle;
    if(api_handle == NULL){
        return -QM_EINVAL;
    }
    return qm_iot_core_event_notify(api_handle->core_handle, QM_IOT_EVENT_RESET);
}

int32_t qm_iot_stop(void *handle)
{
    qm_iot_api_handle_t *api_handle = (qm_iot_api_handle_t *)handle;
    if(api_handle == NULL){
        return -QM_EINVAL;
    }
    
    return qm_iot_core_stop(api_handle->core_handle);
}

int32_t qm_iot_event_notify(void *handle, qm_iot_event_type_t event)
{
    qm_iot_api_handle_t *api_handle = (qm_iot_api_handle_t *)handle;
    if(api_handle == NULL){
        return -QM_EINVAL;
    }
    return qm_iot_core_event_notify(api_handle->core_handle, event);
}

// 清理API句柄和相关资源
int32_t qm_iot_deinit(void **handle)
{
    if(handle == NULL || *handle == NULL) {
        return -QM_EINVAL;
    }
    
    qm_iot_api_handle_t *api_handle = (qm_iot_api_handle_t *)(*handle);
    
    // 停止服务
    qm_iot_stop(api_handle);
    
    // 清理核心句柄
    if(api_handle->core_handle) {
        qm_iot_core_deinit(&api_handle->core_handle);
    }
    
    // 清理API句柄
    qm_free(api_handle);
    *handle = NULL;
    
    return QM_EOK;
}