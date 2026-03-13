#include "qm_iot_weather.h"
#include "qm_kernel.h"
#include "qm_errno.h"
#include "qm_iot_mqtt.h"
#include "qm_log.h"
#include "json_parser.h"
#include "qm_utils_string.h"
#include "qm_string.h"
#include "qm_iot_config.h"

#define LOG_TAG "weather"

#define QM_IOT_WEATHER_TOPIC_MAX_LEN                    48              
#define QM_IOT_WEATHER_REQUEST_TOPIC_FMT                "/%u/%u/device/weather"
#define QM_IOT_WEATHER_REQUEST_PAYLOAD_MAX_LEN          64
#define QM_IOT_WEATHER_REQUEST_PAYLOAD_FMT              "{\"method\":\"weather\",\"id\":%u,\"timestamp\":%u}"

#define QM_IOT_WEATHER_REPLY_TOPIC_FMT                  "/%u/%u/device/weatherReply"

#define DEVICE_PARAMS_KEY                        "params"
#define QM_IOT_WEATHER_CONDITION_KEY             "condition"
#define QM_IOT_WEATHER_ICON_KEY                  "icon"
#define QM_IOT_WEATHER_CITY_KEY                  "city"
#define QM_IOT_WEATHER_TEMP_KEY                  "temp"
#define QM_IOT_WEATHER_HUMI_KEY                  "humidity"
#define QM_IOT_WEATHER_SUNSET_KEY                "sunSet"
#define QM_IOT_WEATHER_SUNRISE_KEY               "sunRise"
#define QM_IOT_WEATHER_CONTRY_NAME_KEY           "counname"
#define QM_IOT_WEATHER_PNAME_KEY                 "pname"
#define QM_IOT_WEATHER_SECONDARY_NAME_KEY        "secondaryname"
#define QM_IOT_WEATHER_NAME_KEY                  "name"
#define QM_IOT_WEATHER_CITY_ID_KEY               "cityId"


#define QM_IOT_WEATHER_MSG_ID_MAX          (65535)

typedef struct {
    uint16_t msg_id;
    void *mqtt_handle;
    void *userdata;   
    qm_iot_weather_recv_handler_t recv_handler;                              
} weather_handle_t;

void *qm_iot_weather_init(void)
{
    weather_handle_t *weather_handle = NULL;

    weather_handle = (weather_handle_t*)qm_malloc(sizeof(weather_handle_t));
    if (weather_handle == NULL) {
        return NULL;
    }
    memset(weather_handle, 0, sizeof(weather_handle_t));
    return weather_handle;
}

static void qm_iot_mqtt_recv_handler(void *handle, const qm_iot_mqtt_recv_t *packet, void *userdata)
{
    int type = 0;
    char *params = NULL;
    int params_len = 0;
    char *condition = NULL;
    int condition_len = 0;
    char *city = NULL;
    int city_len = 0;
    char *value = NULL;
    int value_len = 0;
    uint32_t uint32_value = 0;
    qm_iot_weather_recv_t weather_recv = {0};

    weather_handle_t *weather_handle = (weather_handle_t*)userdata; 

    if(packet->type != QM_IOT_MQTTRECV_PUB){
        return;
    }

    QM_LOGD(LOG_TAG, "payload: %.*s", packet->data.pub.payload_len, packet->data.pub.payload);

    params = qm_json_get_value_by_name((char*)packet->data.pub.payload, (int)packet->data.pub.payload_len, DEVICE_PARAMS_KEY, &params_len, &type);
    if(params == NULL || type != JOBJECT){
        return;
    }

    condition = qm_json_get_value_by_name(params, params_len, QM_IOT_WEATHER_CONDITION_KEY, &condition_len, &type);
    if(condition == NULL || type != JOBJECT){
        return;
    }

    value = qm_json_get_value_by_name(condition, condition_len, QM_IOT_WEATHER_ICON_KEY, &value_len, &type);
    if(value == NULL || type != JSTRING){
        return;
    }
    QM_LOGD(LOG_TAG, "%s: %.*s",  QM_IOT_WEATHER_ICON_KEY, value_len, value);

    qm_str2uint(value, value_len, &uint32_value);
    weather_recv.weather_type = (qm_iot_weather_type_t)uint32_value;

    value = qm_json_get_value_by_name(condition, condition_len, QM_IOT_WEATHER_TEMP_KEY, &value_len, &type);
    if(value == NULL || type != JSTRING){
        return;
    }
    QM_LOGD(LOG_TAG, "%s: %.*s",  QM_IOT_WEATHER_TEMP_KEY, value_len, value);

    qm_str2uint(value, value_len, &uint32_value);
    weather_recv.temp = (int)uint32_value;

    value = qm_json_get_value_by_name(condition, condition_len, QM_IOT_WEATHER_HUMI_KEY, &value_len, &type);
    if(value == NULL || type != JSTRING){
        return;
    }
    QM_LOGD(LOG_TAG, "%s: %.*s",  QM_IOT_WEATHER_HUMI_KEY, value_len, value);

    qm_str2uint(value, value_len, &uint32_value);
    weather_recv.humidity = (int)uint32_value;

    value = qm_json_get_value_by_name(condition, condition_len, QM_IOT_WEATHER_CONDITION_KEY, &value_len, &type);
    if(value == NULL || type != JSTRING){
        return;
    }
    QM_LOGD(LOG_TAG, "%s: %.*s",  QM_IOT_WEATHER_CONDITION_KEY, value_len, value);

    weather_recv.condition = value;
    weather_recv.condition_len = value_len;

    value = qm_json_get_value_by_name(condition, condition_len, QM_IOT_WEATHER_SUNSET_KEY, &value_len, &type);
    if(value == NULL || type != JSTRING){
        return;
    }
    QM_LOGD(LOG_TAG, "%s: %.*s",  QM_IOT_WEATHER_SUNSET_KEY, value_len, value);

    weather_recv.sunset = value;
    weather_recv.sunset_len = value_len;

    value = qm_json_get_value_by_name(condition, condition_len, QM_IOT_WEATHER_SUNRISE_KEY, &value_len, &type);
    if(value == NULL || type != JSTRING){
        return;
    }
    QM_LOGD(LOG_TAG, "%s: %.*s",  QM_IOT_WEATHER_SUNRISE_KEY, value_len, value);

    weather_recv.sunrise = value;
    weather_recv.sunrise_len = value_len;

    city = qm_json_get_value_by_name(params, params_len, QM_IOT_WEATHER_CITY_KEY, &city_len, &type);
    if(city == NULL || type != JOBJECT){
        return;
    }
    QM_LOGD(LOG_TAG, "%s: %.*s",  QM_IOT_WEATHER_CITY_KEY, value_len, value);

    value = qm_json_get_value_by_name(city, city_len, QM_IOT_WEATHER_CONTRY_NAME_KEY, &value_len, &type);
    if(value == NULL || type != JSTRING){
        return;
    }
    QM_LOGD(LOG_TAG, "%s: %.*s",  QM_IOT_WEATHER_CONTRY_NAME_KEY, value_len, value);

    weather_recv.country_name = value;
    weather_recv.country_name_len = value_len;

    value = qm_json_get_value_by_name(city, city_len, QM_IOT_WEATHER_PNAME_KEY, &value_len, &type);
    if(value == NULL || type != JSTRING){
        return;
    }
    QM_LOGD(LOG_TAG, "%s: %.*s",  QM_IOT_WEATHER_PNAME_KEY, value_len, value);

    weather_recv.pname = value;
    weather_recv.pname_len = value_len;

    value = qm_json_get_value_by_name(city, city_len, QM_IOT_WEATHER_SECONDARY_NAME_KEY, &value_len, &type);
    if(value == NULL || type != JSTRING){
        return;
    }
    QM_LOGD(LOG_TAG, "%s: %.*s",  QM_IOT_WEATHER_SECONDARY_NAME_KEY, value_len, value);

    weather_recv.secondaryname = value;
    weather_recv.secondaryname_len = value_len;

    value = qm_json_get_value_by_name(city, city_len, QM_IOT_WEATHER_NAME_KEY, &value_len, &type);
    if(value == NULL || type != JSTRING){
        return;
    }
    weather_recv.name = value;
    QM_LOGD(LOG_TAG, "%s: %.*s",  QM_IOT_WEATHER_NAME_KEY, value_len, value);

    weather_recv.name_len = value_len;

    value = qm_json_get_value_by_name(city, city_len, QM_IOT_WEATHER_CITY_ID_KEY, &value_len, &type);
    if(value == NULL || type != JNUMBER){
        return;
    }
    qm_str2uint(value, value_len, &uint32_value);
    weather_recv.city_id = (int)uint32_value;


    if(weather_handle->recv_handler){
        weather_handle->recv_handler(weather_handle, &weather_recv, weather_handle->userdata);
    }
}

int32_t qm_iot_weather_setopt(void *handle, qm_iot_weather_option_t option, void *data)
{
    qm_err_t ret = QM_EOK;
    char topic[QM_IOT_WEATHER_TOPIC_MAX_LEN] = {0};
    weather_handle_t *weather_handle = (weather_handle_t*)handle; 
    if(handle == NULL){
        return -QM_EINVAL;
    }
    switch(option){

        case QM_IOT_WEATHEROPT_MQTT_HANDLE:
            weather_handle->mqtt_handle = data;
            
            qm_snprintf(topic, QM_IOT_WEATHER_TOPIC_MAX_LEN, QM_IOT_WEATHER_REPLY_TOPIC_FMT, qm_iot_pid_get(), qm_iot_did_get());
            ret = qm_iot_mqtt_pre_sub(weather_handle->mqtt_handle, topic, qm_iot_mqtt_recv_handler, QM_MQTT_QoS0, weather_handle);

        break;
        case QM_IOT_WEATHEROPT_RECV_HANDLER:
            weather_handle->recv_handler = (qm_iot_weather_recv_handler_t)data;

        break;
        case QM_IOT_WEATHEROPT_USERDATA:
            weather_handle->userdata = data;

        break;
        default:
            ret = -QM_EINVAL;
        break;
    }
    return ret;
}

int32_t qm_iot_weather_deinit(void **handle)
{
    char topic[QM_IOT_WEATHER_TOPIC_MAX_LEN] = {0};
    weather_handle_t *weather_handle = (weather_handle_t*)(*handle); 
    if(*handle == NULL){
        return -QM_EINVAL;
    }
    qm_snprintf(topic, QM_IOT_WEATHER_TOPIC_MAX_LEN, QM_IOT_WEATHER_REQUEST_TOPIC_FMT, qm_iot_pid_get(), qm_iot_did_get());
    qm_iot_mqtt_pre_unsub(weather_handle->mqtt_handle, topic);

    qm_free(weather_handle);
    *handle = NULL;

    return QM_EOK;
}

static int qm_iot_weather_msg_id_get(void *handle)
{
    weather_handle_t *weather_handle = (weather_handle_t*)handle; 
    if(handle == NULL){
        return -QM_EINVAL;
    }
    weather_handle->msg_id = (weather_handle->msg_id == QM_IOT_WEATHER_MSG_ID_MAX) ? 1 : weather_handle->msg_id + 1;
    return weather_handle->msg_id;
}

int32_t qm_iot_weather_request(void *handle)
{
    qm_err_t ret = QM_EOK;
    char *payload = NULL;
    char topic[QM_IOT_WEATHER_TOPIC_MAX_LEN] = {0};

    weather_handle_t *weather_handle = (weather_handle_t*)handle; 
    if(handle == NULL){
        return -QM_EINVAL;
    }

    payload = (char*)qm_malloc(QM_IOT_WEATHER_REQUEST_PAYLOAD_MAX_LEN);
    if(payload == NULL){
        return -QM_ENOMEM;
    }
    memset(payload, 0, QM_IOT_WEATHER_REQUEST_PAYLOAD_MAX_LEN);

    qm_snprintf(payload, QM_IOT_WEATHER_REQUEST_PAYLOAD_MAX_LEN, QM_IOT_WEATHER_REQUEST_PAYLOAD_FMT, qm_iot_weather_msg_id_get(handle), qm_now_ms());

    qm_snprintf(topic, QM_IOT_WEATHER_TOPIC_MAX_LEN, QM_IOT_WEATHER_REQUEST_TOPIC_FMT, qm_iot_pid_get(), qm_iot_did_get());

    QM_LOGD(LOG_TAG, "weather[%s] request: %s", topic, payload);
    qm_iot_mqtt_pub(weather_handle->mqtt_handle, topic, (uint8_t*)payload, strlen(payload), QM_MQTT_QoS0);
    
    qm_free(payload);
    payload = NULL;

    return ret;
}