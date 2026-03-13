#include "qm_iot_ntp.h"
#include "qm_kernel.h"
#include "qm_errno.h"
#include "qm_iot_mqtt.h"
#include "qm_log.h"
#include "json_parser.h"
#include "qm_utils_string.h"
#include "qm_string.h"
#include "qm_time.h"
#include "qm_iot_config.h"

#define LOG_TAG "ntp"

#define QM_IOT_NTP_TOPIC_MAX_LEN                    48              
#define QM_IOT_NTP_REQUEST_TOPIC_FMT                "/%u/%u/device/timeSync"
#define QM_IOT_NTP_REQUEST_PAYLOAD_MAX_LEN          128
#define QM_IOT_NTP_REQUEST_PAYLOAD_FMT              "{\"method\":\"timeSync\",\"id\":%u,\"params\":{\"deviceSendTime\":%u}}"

#define QM_IOT_NTP_REPLY_TOPIC_FMT                  "/%u/%u/device/timeSyncReply"

#define DEVICE_PARAMS_KEY              "params"
#define DEVICE_SEND_TIME_KEY           "deviceSendTime"
#define SERVER_RECV_TIME_KEY           "serverRecvTime"
#define SERVER_SEND_TIME_KEY           "serverSendTime"

#define QM_IOT_NTP_MSG_ID_MAX          (65535)

typedef struct {
    uint16_t msg_id;
    void *mqtt_handle;
    int8_t time_zone;
    void *userdata;   
    qm_iot_ntp_recv_handler_t recv_handler;                              
} ntp_handle_t;

void *qm_iot_ntp_init(void)
{
    ntp_handle_t *ntp_handle = NULL;

    ntp_handle = (ntp_handle_t*)qm_malloc(sizeof(ntp_handle_t));
    if (ntp_handle == NULL) {
        return NULL;
    }
    memset(ntp_handle, 0, sizeof(ntp_handle_t));
    return ntp_handle;
}

static void qm_iot_mqtt_recv_handler(void *handle, const qm_iot_mqtt_recv_t *packet, void *userdata)
{
    int type = 0;
    char *params = NULL;
    int params_len = 0;

    char *value = NULL;
    int value_len = 0;
    struct qm_tm tm = {0};
    qm_time_t time = 0;
    qm_time_t pre_time = 0;
    qm_iot_ntp_recv_t ntp_recv = {0};
    ntp_handle_t *ntp_handle = (ntp_handle_t*)userdata; 
    uint64_t device_send_time = 0, server_send_time = 0, server_recv_time = 0;

    if(packet->type != QM_IOT_MQTTRECV_PUB){
        return;
    }

    QM_LOGD(LOG_TAG, "payload: %.*s", packet->data.pub.payload_len, packet->data.pub.payload);

    params = qm_json_get_value_by_name((char*)packet->data.pub.payload, (int)packet->data.pub.payload_len, DEVICE_PARAMS_KEY, &params_len, &type);
    if(params == NULL || type != JOBJECT){
        return;
    }

    value = qm_json_get_value_by_name(params, params_len, DEVICE_SEND_TIME_KEY, &value_len, &type);
    if(value == NULL || type != JNUMBER){
        return;
    }
    qm_str2uint64(value, value_len, &device_send_time);

    value = qm_json_get_value_by_name(params, params_len, SERVER_RECV_TIME_KEY, &value_len, &type);
    if(value == NULL || type != JNUMBER){
        return;
    }
    qm_str2uint64(value, value_len, &server_recv_time);

    value = qm_json_get_value_by_name(params, params_len, SERVER_SEND_TIME_KEY, &value_len, &type);
    if(value == NULL || type != JNUMBER){
        return;
    }
    qm_str2uint64(value, value_len, &server_send_time);

    pre_time = ((server_send_time + server_recv_time + qm_now_ms() - device_send_time) / 2 / 1000);

    time = pre_time + ntp_handle->time_zone * 3600;
    
    qm_gmtime_r(&time, &tm);
    
    ntp_recv.type = QM_IOT_NTPRECV_LOCAL_TIME;
    ntp_recv.data.local_time.timestamp = (uint64_t)pre_time;
    ntp_recv.data.local_time.year = tm.tm_year + 1900;
    ntp_recv.data.local_time.mon = tm.tm_mon + 1;
    ntp_recv.data.local_time.day = tm.tm_mday;
    ntp_recv.data.local_time.hour = tm.tm_hour;
    ntp_recv.data.local_time.min = tm.tm_min;
    ntp_recv.data.local_time.sec = tm.tm_sec;
    ntp_recv.data.local_time.msec = (uint16_t)(time % 1000);

    if(ntp_handle->recv_handler){
        ntp_handle->recv_handler(ntp_handle, &ntp_recv, ntp_handle->userdata);
    }
}

int32_t qm_iot_ntp_setopt(void *handle, qm_iot_ntp_option_t option, void *data)
{
    qm_err_t ret = QM_EOK;
    char topic[QM_IOT_NTP_TOPIC_MAX_LEN] = {0};
    ntp_handle_t *ntp_handle = (ntp_handle_t*)handle; 
    if(handle == NULL){
        return -QM_EINVAL;
    }
    switch(option){

        case QM_IOT_NTPOPT_MQTT_HANDLE:
            ntp_handle->mqtt_handle = data;
            
            qm_snprintf(topic, QM_IOT_NTP_TOPIC_MAX_LEN, QM_IOT_NTP_REPLY_TOPIC_FMT, qm_iot_pid_get(), qm_iot_did_get());
            ret = qm_iot_mqtt_pre_sub(ntp_handle->mqtt_handle, topic, qm_iot_mqtt_recv_handler, QM_MQTT_QoS0, ntp_handle);

        break;
        case QM_IOT_NTPOPT_TIME_ZONE:
            ntp_handle->time_zone = *((int8_t*)data);

        break;
        case QM_IOT_NTPOPT_RECV_HANDLER:
            ntp_handle->recv_handler = (qm_iot_ntp_recv_handler_t)data;

        break;
        case QM_IOT_NTPOPT_USERDATA:
            ntp_handle->userdata = data;

        break;
        default:
            ret = -QM_EINVAL;
        break;
    }
    return ret;
}

int32_t qm_iot_ntp_deinit(void **handle)
{
    char topic[QM_IOT_NTP_TOPIC_MAX_LEN] = {0};
    ntp_handle_t *ntp_handle = (ntp_handle_t*)(*handle); 
    if(*handle == NULL){
        return -QM_EINVAL;
    }
    qm_snprintf(topic, QM_IOT_NTP_TOPIC_MAX_LEN, QM_IOT_NTP_REQUEST_TOPIC_FMT, qm_iot_pid_get(), qm_iot_did_get());
    qm_iot_mqtt_pre_unsub(ntp_handle->mqtt_handle, topic);

    qm_free(ntp_handle);
    *handle = NULL;

    return QM_EOK;
}

static int qm_iot_ntp_msg_id_get(void *handle)
{
    ntp_handle_t *ntp_handle = (ntp_handle_t*)handle; 
    if(handle == NULL){
        return -QM_EINVAL;
    }
    ntp_handle->msg_id = (ntp_handle->msg_id == QM_IOT_NTP_MSG_ID_MAX) ? 1 : ntp_handle->msg_id + 1;
    return ntp_handle->msg_id;
}

int32_t qm_iot_ntp_request(void *handle)
{
    qm_err_t ret = QM_EOK;
    char *payload = NULL;
    char topic[QM_IOT_NTP_TOPIC_MAX_LEN] = {0};

    ntp_handle_t *ntp_handle = (ntp_handle_t*)handle; 
    if(handle == NULL){
        return -QM_EINVAL;
    }

    payload = (char*)qm_malloc(QM_IOT_NTP_REQUEST_PAYLOAD_MAX_LEN);
    if(payload == NULL){
        return -QM_ENOMEM;
    }
    memset(payload, 0, QM_IOT_NTP_REQUEST_PAYLOAD_MAX_LEN);

    qm_snprintf(payload, QM_IOT_NTP_REQUEST_PAYLOAD_MAX_LEN, QM_IOT_NTP_REQUEST_PAYLOAD_FMT, qm_iot_ntp_msg_id_get(handle), qm_now_ms());

    qm_snprintf(topic, QM_IOT_NTP_TOPIC_MAX_LEN, QM_IOT_NTP_REQUEST_TOPIC_FMT, qm_iot_pid_get(), qm_iot_did_get());

    QM_LOGD(LOG_TAG, "ntp[%s] request: %s", topic, payload);
    qm_iot_mqtt_pub(ntp_handle->mqtt_handle, topic, (uint8_t*)payload, strlen(payload), QM_MQTT_QoS0);
    
    qm_free(payload);
    payload = NULL;

    return ret;
}