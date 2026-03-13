#include "qm.h"
#include "qm_time.h"
#include "qm_iot_dev.h"
#include "qm_iot_common.h"
#include "qm_iot_config.h"
#include "qm_work.h"
#include "cJSON.h"
#include "json_parser.h"
#include "qm_utils_string.h"

#if CONFIG_QM_IOT_NETWORK_TYPE_WIFI
#include "qm_wifi.h"
#endif


#define LOG_TAG  "subdev"

#if CONFIG_QM_IOT_DEVMNT_SUPPORT
#include "qm_iot_mqtt.h"

#define QM_IOT_DEV_SUB_RETRY_COUNT                  (10)

#define QM_IOT_DEV_RESET_RETRY_TIMEOUT              (10 * 1000)

#define QM_IOT_DEV_TOPIC_MAX_LEN                     64           
#define QM_IOT_DEV_PARAMS_PAYLOAD_MAX_LEN            64
#define QM_IOT_DEV_GENERIC_PAYLOAD_MAX_LEN           256   

#define QM_IOT_BIND_DEV_TOPIC_FMT                  "/%u/%u/device/bindDevNotify"
#define QM_IOT_BIND_DEV_REPLY_TOPIC_FMT            "/%u/%u/device/bindDevNotifyReply"
#define QM_IOT_DEV_BIND_NOTIFY_REPLY_PAYLOAD_FMT  "{\"method\":\"bindDevNotifyReply\",\"id\":%d,\"code\":0,\"timestamp\":%u}"

#define QM_IOT_DEV_DELDEV_TOPIC_FMT                  "/%u/%u/device/delDev"
#define QM_IOT_DEV_DELDEV_REPLY_TOPIC_FMT            "/%u/%u/device/delDevReply"
#define QM_IOT_DEV_UNBIND_TOPIC_FMT                  "/%u/%u/device/delDevNotify"
#define QM_IOT_DEV_UNBIND_REPLY_TOPIC_FMT            "/%u/%u/device/delDevNotifyReply"

#define QM_IOT_DEV_RESET_NOTIFY_PAYLOAD_FMT         "{\"method\":\"delDev\",\"id\":%d,\"timestamp\":%u}"
#define QM_IOT_DEV_UNBIND_NOTIFY_REPLY_PAYLOAD_FMT  "{\"method\":\"delDevNotifyReply\",\"id\":%d,\"code\":0,\"timestamp\":%u}"

#if CONFIG_QM_IOT_DEV_IR_SUPPORT
#define QM_IOT_IR_TOPIC_FMT                             "/%u/%u/device/sendIRMessage" 
#define QM_IOT_IR_RPLAY_TOPIC_FMT                       "/%u/%u/device/sendIRMessageReply" 
#define QM_IOT_IR_RPLAY_PARAMS                      "{\"method\":\"sendIRMessageReply\",\"id\":%d,\"timestamp\":%u,\"code\":%d}"
#endif

#if CONFIG_QM_IOT_DEV_INFO_SUPPORT

#define QM_IOT_DEV_INFO_TOPIC_FMT                   "/%u/%u/device/devInfo" 
#define QM_IOT_DEV_INFO_RPLAY_TOPIC_FMT             "/%u/%u/device/devInfoReply" 
#define QM_IOT_DEV_INFO_PARAMS                      "{\"method\":\"devInfo\",\"id\":%d, \"params\":%s,\"timestamp\":%u}"

#define TYPE_KEY                    "type"
#define MAC_TYPE_KEY                "mac"
#define SSID_TYPE_KEY               "ssid"
#define BSSID_TYPE_KEY              "bssid"
#define CHANNEL_TYPE_KEY            "channel"
#define RSSI_TYPE_KEY               "rssi"
#define IP_TYPE_KEY                 "ip"
#define MODULE_TYPE_KEY             "moudle"

#define WIFI_KEY                    "wifi"
#define PARAM_4G_KEY                "4g"

#define PARAM_LONGITUDE_KEY         "longitude"
#define PARAM_LATITUDE_KEY          "latitude"
#define PARAM_ICCID_KEY             "iccId"

#endif

#define QM_IOT_DEV_MSG_ID_MAX          (65535)

#define PARAMS_KEY                  "params"
#define MESSAGE_KEY                 "message"
#define METHOD_KEY                  "method"
#define CODE_KEY                    "code"
#define MSG_ID_KEY                  "id"
#define TIMESTAMP_KEY               "timestamp"

typedef struct {
    qm_iot_dev_recv_type_t  recv_type;
    char *params;
    qm_iot_mqtt_recv_handler_t recv_handler;
    char topic[QM_IOT_DEV_TOPIC_MAX_LEN];
}dev_handler_t;

typedef struct {
    uint16_t msg_id;
    void *mqtt_handle;
    void *userdata;   
    qm_iot_dev_recv_handler_t recv_handler;   
    qm_work_t retry_work;                    
} dev_handle_t;

static dev_handle_t *g_dev_handle = NULL;

#if CONFIG_QM_IOT_DEV_IR_SUPPORT
static void mqtt_ir_recv_handler(void *handle, const qm_iot_mqtt_recv_t *packet, void *userdata);
#endif

#if CONFIG_QM_IOT_DEV_INFO_SUPPORT
static void mqtt_devinfo_reply_recv_handler(void *handle, const qm_iot_mqtt_recv_t *packet, void *userdata);
#endif

static void qm_iot_dev_unbind_reply(void *handle);
static void mqtt_reset_reply_recv_handler(void *handle, const qm_iot_mqtt_recv_t *packet, void *userdata);
static void mqtt_bind_notify_recv_handler(void *handle, const qm_iot_mqtt_recv_t *packet, void *userdata);
static void mqtt_unbind_notify_recv_handler(void *handle, const qm_iot_mqtt_recv_t *packet, void *userdata);

void *qm_iot_dev_init(void)
{
    dev_handle_t *dev_handle = NULL;

    dev_handle = (dev_handle_t*)qm_malloc(sizeof(dev_handle_t));
    if (dev_handle == NULL) {
        return NULL;
    }
    memset(dev_handle, 0, sizeof(dev_handle_t));

    g_dev_handle = dev_handle;
    return dev_handle;
}

static int qm_iot_dev_msg_id_get(void *handle)
{
    dev_handle_t *subdev_handle = (dev_handle_t*)handle; 
    if(handle == NULL){
        return -QM_EINVAL;
    }
    subdev_handle->msg_id = (subdev_handle->msg_id == QM_IOT_DEV_MSG_ID_MAX) ? 1 : subdev_handle->msg_id + 1;
    return subdev_handle->msg_id;
}

static void qm_iot_dev_sub_topic(dev_handle_t *subdev_handle)
{
    int ret = QM_EOK;
    char topic[QM_IOT_DEV_TOPIC_MAX_LEN] = {0};
    qm_snprintf(topic, QM_IOT_DEV_TOPIC_MAX_LEN, QM_IOT_DEV_UNBIND_TOPIC_FMT, qm_iot_pid_get(), qm_iot_did_get());
    qm_iot_mqtt_pre_sub(subdev_handle->mqtt_handle, topic, mqtt_unbind_notify_recv_handler, QM_MQTT_QoS0, subdev_handle);
    memset(topic, 0, QM_IOT_DEV_TOPIC_MAX_LEN);
    qm_snprintf(topic, QM_IOT_DEV_TOPIC_MAX_LEN, QM_IOT_DEV_DELDEV_REPLY_TOPIC_FMT, qm_iot_pid_get(), qm_iot_did_get());
    qm_iot_mqtt_pre_sub(subdev_handle->mqtt_handle, topic, mqtt_reset_reply_recv_handler, QM_MQTT_QoS0, subdev_handle);

    qm_snprintf(topic, QM_IOT_DEV_TOPIC_MAX_LEN, QM_IOT_BIND_DEV_TOPIC_FMT, qm_iot_pid_get(), qm_iot_did_get());
    qm_iot_mqtt_pre_sub(subdev_handle->mqtt_handle, topic, mqtt_bind_notify_recv_handler, QM_MQTT_QoS0, subdev_handle);

#if CONFIG_QM_IOT_DEV_IR_SUPPORT
    memset(topic, 0, QM_IOT_DEV_TOPIC_MAX_LEN);
    qm_snprintf(topic, QM_IOT_DEV_TOPIC_MAX_LEN, QM_IOT_IR_TOPIC_FMT, qm_iot_pid_get(), qm_iot_did_get());
    qm_iot_mqtt_pre_sub(subdev_handle->mqtt_handle, topic, mqtt_ir_recv_handler, QM_MQTT_QoS0, subdev_handle);
#endif

#if CONFIG_QM_IOT_DEV_INFO_SUPPORT
    memset(topic, 0, QM_IOT_DEV_TOPIC_MAX_LEN);
    qm_snprintf(topic, QM_IOT_DEV_TOPIC_MAX_LEN, QM_IOT_DEV_INFO_RPLAY_TOPIC_FMT, qm_iot_pid_get(), qm_iot_did_get());
    qm_iot_mqtt_pre_sub(subdev_handle->mqtt_handle, topic, mqtt_devinfo_reply_recv_handler, QM_MQTT_QoS0, subdev_handle);
#endif

}

#if 0
static void qm_iot_dev_unsub_topic(void *mqtt_handle)
{
    for(int i = 0; i < QM_ARRAY_SIZE(g_dev_handler); i++)
    {
        qm_snprintf(g_dev_handler[i].topic, QM_IOT_DEV_TOPIC_MAX_LEN, g_dev_handler[i].params, qm_iot_pid_get(), qm_iot_did_get());
        qm_iot_mqtt_unsub(mqtt_handle, g_dev_handler[i].topic);
    }
}
#endif

#if CONFIG_QM_IOT_DEV_INFO_SUPPORT

//暂不实现
static void mqtt_devinfo_reply_recv_handler(void *handle, const qm_iot_mqtt_recv_t *packet, void *userdata)
{
    
}

#if CONFIG_QM_IOT_NETWORK_TYPE_WIFI
char *qm_iot_dev_wifi_devinfo_pack(void)
{
    uint8_t mac[6] = {0};
    char *payload = NULL;
    qm_ip_info_t ip_info = {0};
    qm_wifi_ap_record_t ap_info = {0};
    char temp_value[QM_IOT_DEV_PARAMS_PAYLOAD_MAX_LEN] = {0};
    cJSON *root = cJSON_CreateObject();
    if(root == NULL){
        goto __exit;
    }
    cJSON_AddStringToObject(root, TYPE_KEY, WIFI_KEY);

    qm_wifi_get_mac(QM_WIFI_IF_STA, mac);
    memset(temp_value, 0, QM_IOT_DEV_PARAMS_PAYLOAD_MAX_LEN);
    qm_snprintf(temp_value, QM_IOT_DEV_PARAMS_PAYLOAD_MAX_LEN, 
                "%02X:%02X:%02X:%02X:%02X:%02X", 
                mac[0], mac[1], mac[2],mac[3],mac[4],mac[5]);
    cJSON_AddStringToObject(root, MAC_TYPE_KEY, temp_value); 

    qm_wifi_sta_get_ap_info(&ap_info);
    memset(temp_value, 0, QM_IOT_DEV_PARAMS_PAYLOAD_MAX_LEN);
    qm_snprintf(temp_value, QM_IOT_DEV_PARAMS_PAYLOAD_MAX_LEN,
                "%02X:%02X:%02X:%02X:%02X:%02X", 
                ap_info.bssid[0], ap_info.bssid[1], ap_info.bssid[2],
                ap_info.bssid[3],ap_info.bssid[4],ap_info.bssid[5]);

    cJSON_AddNumberToObject(root, RSSI_TYPE_KEY, (double)ap_info.rssi); 
    cJSON_AddStringToObject(root, BSSID_TYPE_KEY, temp_value); 
    cJSON_AddStringToObject(root, SSID_TYPE_KEY, (char *)ap_info.ssid); 
    cJSON_AddNumberToObject(root, CHANNEL_TYPE_KEY, (double)ap_info.channel);

    qm_iot_wifi_ip_info_get(&ip_info);
    memset(temp_value, 0, QM_IOT_DEV_PARAMS_PAYLOAD_MAX_LEN);
    qm_snprintf(temp_value, QM_IOT_DEV_PARAMS_PAYLOAD_MAX_LEN, 
                QM_IPSTR, QM_IP2STR(&ip_info.ip));    
    cJSON_AddStringToObject(root, IP_TYPE_KEY, temp_value); 
    
    payload = cJSON_PrintUnformatted(root);
    if(payload == NULL){
        goto __exit;
    }

__exit:
    if(root){
        cJSON_Delete(root);
        root = NULL;
    }
    return payload;
}
#endif


#if CONFIG_QM_IOT_NETWORK_TYPE_4G
char *qm_iot_dev_4g_devinfo_pack(void)
{
    char imsi[64];
    char *payload = NULL;
    qm_modem_lbs_t lbs_info = {0};
    cJSON *root = cJSON_CreateObject();
    if(root == NULL){
        goto __exit;
    }
    cJSON_AddStringToObject(root, TYPE_KEY, PARAM_4G_KEY);

    qm_modem_get_imsi(NULL, imsi, 64);
    if(imsi[0] == '\0'){
        goto __exit;
    }

    qm_iot_4g_lbs_info_get(&lbs_info);

    cJSON_AddStringToObject(root, PARAM_ICCID_KEY, imsi); 
    cJSON_AddStringToObject(root, PARAM_LONGITUDE_KEY, lbs_info.longitude); 
    cJSON_AddStringToObject(root, PARAM_LATITUDE_KEY, lbs_info.latitude); 

    payload = cJSON_PrintUnformatted(root);
    if(payload == NULL){
        goto __exit;
    }

__exit:
    if(root){
        cJSON_Delete(root);
        root = NULL;
    }
    return payload;
}
#endif



int32_t qm_iot_dev_send_devinfo(void *handle)
{
    char topic[QM_IOT_DEV_TOPIC_MAX_LEN] = {0};
    char *params = NULL;
    char *payload = NULL;
    qm_time_t timestamp = 0;
    dev_handle_t *dev_handle = (dev_handle_t*)handle; 
    if(handle == NULL){
        return -QM_EINVAL;
    }

    payload = (char*)qm_malloc(QM_IOT_DEV_GENERIC_PAYLOAD_MAX_LEN);
    if(payload == NULL){
        return -QM_ENOMEM;

    }
    memset(payload, 0, QM_IOT_DEV_GENERIC_PAYLOAD_MAX_LEN);

#if CONFIG_QM_IOT_NETWORK_TYPE_WIFI
    params =  qm_iot_dev_wifi_devinfo_pack();
    if(params == NULL){
        goto __exit;
    }
#endif

#if CONFIG_QM_IOT_NETWORK_TYPE_4G
    params =  qm_iot_dev_4g_devinfo_pack();
    if(params == NULL){
        goto __exit;
    }
#endif

    qm_time(&timestamp);
    qm_snprintf(topic, QM_IOT_DEV_TOPIC_MAX_LEN, QM_IOT_DEV_INFO_TOPIC_FMT, qm_iot_pid_get(), qm_iot_did_get());
    qm_snprintf(payload, QM_IOT_DEV_GENERIC_PAYLOAD_MAX_LEN, 
                QM_IOT_DEV_INFO_PARAMS, qm_iot_dev_msg_id_get(dev_handle), params, timestamp);    

    qm_iot_mqtt_pub(dev_handle->mqtt_handle, topic, (uint8_t *)payload, strlen(payload), QM_MQTT_QoS0);
    QM_LOGD(LOG_TAG, "pub devinfo[%s]:%s", topic, payload);
    
__exit:

    if(params){
        qm_free(params);
        params = NULL;
    }

    qm_free(payload);
    payload = NULL;

    return QM_EOK;
}


#endif


#if CONFIG_QM_IOT_DEV_IR_SUPPORT
static void qm_iot_dev_ir_reply(void *handle, uint32_t msg_id, int code)
{
    char topic[QM_IOT_DEV_TOPIC_MAX_LEN] = {0};
    char *payload = NULL;
    dev_handle_t *dev_handle = (dev_handle_t*)handle; 
    if(handle == NULL){
        return;
    }

    payload = (char*)qm_malloc(QM_IOT_DEV_GENERIC_PAYLOAD_MAX_LEN);
    if(payload == NULL){
        return ;
    }
    memset(payload, 0, QM_IOT_DEV_GENERIC_PAYLOAD_MAX_LEN);

    qm_snprintf(topic, QM_IOT_DEV_TOPIC_MAX_LEN, QM_IOT_IR_RPLAY_TOPIC_FMT, qm_iot_pid_get(), qm_iot_did_get());
    qm_snprintf(payload, QM_IOT_DEV_GENERIC_PAYLOAD_MAX_LEN, QM_IOT_IR_RPLAY_PARAMS, msg_id, qm_now_ms(), code);
    
    qm_iot_mqtt_pub(dev_handle->mqtt_handle, topic, (uint8_t *)payload, strlen(payload), QM_MQTT_QoS0);
    QM_LOGD(LOG_TAG, "pub ir reply[%s]:%s", topic, payload);
    qm_free(payload);
    payload = NULL;
}

static void mqtt_ir_recv_handler(void *handle, const qm_iot_mqtt_recv_t *packet, void *userdata)
{
    int type = 0;

    char *str_id = NULL;
    int id_len = 0;

    char *str_params = NULL;
    int params_len = 0;

    char *str_message = NULL;
    int message_len = 0;

    qm_iot_dev_recv_t dev_recv = {0};
    dev_handle_t *dev_handle = (dev_handle_t*)userdata; 
    if(dev_handle == NULL){
        return;
    }

    if(packet->type != QM_IOT_MQTTRECV_PUB){
        return ;
    }
    
    if(g_dev_handle == NULL){
        return ;
    }

    dev_recv.type = QM_IOT_DEVRECV_IR_NOTIFY;

    QM_LOGD(LOG_TAG, "recv ir notify:%.*s", packet->data.pub.payload_len, packet->data.pub.payload);
    str_id = qm_json_get_value_by_name((char *)packet->data.pub.payload, packet->data.pub.payload_len, MSG_ID_KEY, &id_len, &type);
    dev_recv.data.ir_notify.msg_id = int_str_to_num(str_id, id_len);

    str_params = qm_json_get_value_by_name((char *)packet->data.pub.payload, packet->data.pub.payload_len, PARAMS_KEY, &params_len, &type);
    if(str_params == NULL){
        QM_LOGW(LOG_TAG, "ir params failed !!");
        return;
    }

    str_message = qm_json_get_value_by_name(str_params, params_len, MESSAGE_KEY, &message_len, &type);
    if(str_message == NULL){
        QM_LOGW(LOG_TAG, "ir str_message failed !!");
        return;
    }

    dev_recv.data.ir_notify.data.msg = str_message;
    dev_recv.data.ir_notify.msg_len = message_len;

    if(g_dev_handle->recv_handler){
        g_dev_handle->recv_handler(g_dev_handle, &dev_recv, g_dev_handle->userdata);
    }

    qm_iot_dev_ir_reply(g_dev_handle, dev_recv.data.ir_notify.msg_id, (int)QM_IOT_CODE_SUCCESS);
}
#endif

static void qm_iot_dev_reset_request_retry(void *arg)
{
    qm_iot_dev_reset_request(arg);
}

static void qm_iot_dev_unbind_reply(void *handle)
{
    char topic[QM_IOT_DEV_TOPIC_MAX_LEN] = {0};
    char *payload = NULL;
    dev_handle_t *dev_handle = (dev_handle_t*)handle; 
    if(handle == NULL){
        return;
    }

    payload = (char*)qm_malloc(QM_IOT_DEV_GENERIC_PAYLOAD_MAX_LEN);
    if(payload == NULL){
        return ;
    }
    memset(payload, 0, QM_IOT_DEV_GENERIC_PAYLOAD_MAX_LEN);

    qm_snprintf(topic, QM_IOT_DEV_TOPIC_MAX_LEN, QM_IOT_DEV_UNBIND_REPLY_TOPIC_FMT, qm_iot_pid_get(), qm_iot_did_get());
    qm_snprintf(payload, QM_IOT_DEV_GENERIC_PAYLOAD_MAX_LEN, QM_IOT_DEV_UNBIND_NOTIFY_REPLY_PAYLOAD_FMT, qm_iot_dev_msg_id_get(dev_handle), qm_now_ms());
    
    qm_iot_mqtt_pub(dev_handle->mqtt_handle, topic, (uint8_t *)payload, strlen(payload), QM_MQTT_QoS0);

    qm_free(payload);
    payload = NULL;
}

static void mqtt_reset_reply_recv_handler(void *handle, const qm_iot_mqtt_recv_t *packet, void *userdata)
{
    int type = 0;
    char *str_id = NULL;
    int id_len = 0;
    int code = 0;
    char *str_code = NULL;
    int code_len = 0;
    qm_iot_dev_recv_t dev_recv = {0};
    dev_handle_t *dev_handle = (dev_handle_t*)userdata; 
    if(dev_handle == NULL){
        return;
    }

    if(packet->type != QM_IOT_MQTTRECV_PUB){
        return ;
    }
    
    dev_recv.type = QM_IOT_DEVRECV_RESET_REPLY;

    QM_LOGD(LOG_TAG, "recv reply notify:%.*s", packet->data.pub.payload_len, packet->data.pub.payload);
    str_id = qm_json_get_value_by_name((char *)packet->data.pub.payload, packet->data.pub.payload_len, MSG_ID_KEY, &id_len, &type);
    dev_recv.data.reset_reply.msg_id = int_str_to_num(str_id, id_len);

    str_code = qm_json_get_value_by_name((char *)packet->data.pub.payload, packet->data.pub.payload_len, CODE_KEY, &code_len, &type);
    code = int_str_to_num(str_code, code_len);
    if(code != QM_IOT_CODE_SUCCESS){
        QM_LOGW(LOG_TAG, "dev unbind failed ,need retry!!");
        return;
    }

    qm_cancel_delayed_action(&dev_handle->retry_work);
    dev_recv.data.reset_reply.code = QM_IOT_CODE_SUCCESS;
    if(dev_handle->recv_handler){
        dev_handle->recv_handler(dev_handle, &dev_recv, dev_handle->userdata);
    }
}

static void mqtt_unbind_notify_recv_handler(void *handle, const qm_iot_mqtt_recv_t *packet, void *userdata)
{
    int type = 0;
    char *str_id = NULL;
    int id_len = 0;
    qm_iot_dev_recv_t dev_recv = {0};
    dev_handle_t *dev_handle = (dev_handle_t*)userdata; 

    if(dev_handle == NULL){
        return;
    }

    if(packet->type != QM_IOT_MQTTRECV_PUB){
        return ;
    }
    
    dev_recv.type = QM_IOT_DEVRECV_UNBIND_NOTIFY;
    QM_LOGD(LOG_TAG, "recv unbind notify:%.*s", packet->data.pub.payload_len, packet->data.pub.payload);

    str_id = qm_json_get_value_by_name((char *)packet->data.pub.payload, packet->data.pub.payload_len, MSG_ID_KEY, &id_len, &type);
    dev_recv.data.reset_reply.msg_id = int_str_to_num(str_id, id_len);

    qm_iot_dev_unbind_reply(dev_handle);

    dev_recv.data.reset_reply.code = QM_IOT_CODE_SUCCESS;
    if(dev_handle->recv_handler){
        dev_handle->recv_handler(dev_handle, &dev_recv, dev_handle->userdata);
    }
}

static void qm_iot_dev_bind_reply(void *handle)
{
    qm_time_t time = 0;
    char topic[QM_IOT_DEV_TOPIC_MAX_LEN] = {0};
    char *payload = NULL;
    dev_handle_t *dev_handle = (dev_handle_t*)handle; 
    if(handle == NULL){
        return;
    }

    payload = (char*)qm_malloc(QM_IOT_DEV_GENERIC_PAYLOAD_MAX_LEN);
    if(payload == NULL){
        return ;
    }
    memset(payload, 0, QM_IOT_DEV_GENERIC_PAYLOAD_MAX_LEN);

    qm_time(&time);
    qm_snprintf(topic, QM_IOT_DEV_TOPIC_MAX_LEN, QM_IOT_BIND_DEV_REPLY_TOPIC_FMT, qm_iot_pid_get(), qm_iot_did_get());
    qm_snprintf(payload, QM_IOT_DEV_GENERIC_PAYLOAD_MAX_LEN, QM_IOT_DEV_BIND_NOTIFY_REPLY_PAYLOAD_FMT, qm_iot_dev_msg_id_get(dev_handle), (uint32_t)time);
    
    qm_iot_mqtt_pub(dev_handle->mqtt_handle, topic, (uint8_t *)payload, strlen(payload), QM_MQTT_QoS0);
    QM_LOGD(LOG_TAG, "bind reply: %s", payload);

    qm_free(payload);
    payload = NULL;
}

static void mqtt_bind_notify_recv_handler(void *handle, const qm_iot_mqtt_recv_t *packet, void *userdata)
{
    int type = 0;
    char *str_id = NULL;
    int id_len = 0;
    qm_iot_dev_recv_t dev_recv = {0};
    dev_handle_t *dev_handle = (dev_handle_t*)userdata; 

    if(dev_handle == NULL){
        return;
    }

    if(packet->type != QM_IOT_MQTTRECV_PUB){
        return ;
    }
    
    dev_recv.type = QM_IOT_DEVRECV_BIND_NOTIFY;
    QM_LOGD(LOG_TAG, "recv bind notify:%.*s", packet->data.pub.payload_len, packet->data.pub.payload);

    str_id = qm_json_get_value_by_name((char *)packet->data.pub.payload, packet->data.pub.payload_len, MSG_ID_KEY, &id_len, &type);
    dev_recv.data.bind_notify.msg_id = int_str_to_num(str_id, id_len);

    qm_iot_dev_bind_reply(dev_handle);

    if(dev_handle->recv_handler){
        dev_handle->recv_handler(dev_handle, &dev_recv, dev_handle->userdata);
    }
}

int32_t qm_iot_dev_setopt(void *handle, qm_iot_dev_option_t option, void *data)
{
    qm_err_t ret = QM_EOK;
    dev_handle_t *dev_handle = (dev_handle_t*)handle; 
    if(handle == NULL){
        return -QM_EINVAL;
    }
    switch(option){

        case QM_IOT_DEVOPT_MQTT_HANDLE:
            dev_handle->mqtt_handle = data;
            qm_iot_dev_sub_topic(dev_handle);
        break;
        case QM_IOT_DEVOPT_RECV_HANDLER:
            dev_handle->recv_handler = (qm_iot_dev_recv_handler_t)data;
        break;
        case QM_IOT_DEVOPT_USERDATA:
            dev_handle->userdata = data;
        break;
        default:
            ret = -QM_EINVAL;
        break;
    }
    return ret;
}

int32_t qm_iot_dev_deinit(void **handle)
{
    dev_handle_t *dev_handle = (dev_handle_t*)(*handle); 
    if(handle == NULL){
        return -QM_EINVAL;
    }

    g_dev_handle = NULL;
#if 0
    qm_iot_dev_unsub_topic(dev_handle->mqtt_handle);
#endif
    qm_cancel_delayed_action(&dev_handle->retry_work);

    qm_free(dev_handle);
    *handle = NULL;

    return QM_EOK;
}

int32_t qm_iot_dev_reset_request(void *handle)
{
    char *payload = NULL;
    char topic[QM_IOT_DEV_TOPIC_MAX_LEN] = {0};
    dev_handle_t *dev_handle = (dev_handle_t*)handle; 
    if(handle == NULL){
        return -QM_EINVAL;
    }

    payload = (char*)qm_malloc(QM_IOT_DEV_GENERIC_PAYLOAD_MAX_LEN);
    if(payload == NULL){
        return -QM_ENOMEM;
    }
    memset(payload, 0, QM_IOT_DEV_GENERIC_PAYLOAD_MAX_LEN);

    qm_cancel_delayed_action(&dev_handle->retry_work);

    qm_snprintf(topic, QM_IOT_DEV_TOPIC_MAX_LEN, QM_IOT_DEV_DELDEV_TOPIC_FMT, qm_iot_pid_get(), qm_iot_did_get());
    qm_snprintf(payload, QM_IOT_DEV_GENERIC_PAYLOAD_MAX_LEN, QM_IOT_DEV_RESET_NOTIFY_PAYLOAD_FMT, qm_iot_dev_msg_id_get(dev_handle), qm_now_ms());
    
    QM_LOGD(LOG_TAG, "unbind notify[%s]:%s", topic, payload);
    qm_iot_mqtt_pub(dev_handle->mqtt_handle, topic, (uint8_t *)payload, strlen(payload), QM_MQTT_QoS0);

    qm_free(payload);
    payload = NULL;

    qm_post_delayed_action(&dev_handle->retry_work, qm_iot_dev_reset_request_retry, dev_handle, QM_IOT_DEV_RESET_RETRY_TIMEOUT);
    return QM_EOK;
}

#endif