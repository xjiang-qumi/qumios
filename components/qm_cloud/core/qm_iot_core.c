#include "qm.h"
#include "qm_work.h"
#include "qm_iot_api.h"
#include "qm_iot_core.h"
#include "qm_iot_config.h"
#include "qm_spec_api.h"
#include "qm_spec_core.h"

#ifdef CONFIG_QM_IOT_PROV_SUPPORT
#include "qm_iot_prov.h"
#endif

#if CONFIG_QM_IOT_NETWORK_TYPE_WIFI
#include "qm_wifi.h"
#include "qm_ble_gap.h"
#endif

#if CONFIG_QM_IOT_NETWORK_TYPE_4G
#include "qm_modem.h"
#endif

#define LOG_TAG "IOT_CORE"

#define CONFIG_QM_IOT_RESET_DELAY_MS            (200)
#define CONFIG_QM_IOT_RESET_FORCE_MS            (5000)

#if CONFIG_QM_IOT_CORE_WEATHER_SUPPORT
#include "qm_iot_weather.h"
#define CONFIG_QM_IOT_WEATHER_DELAY_MS          (2000)
#endif

#if CONFIG_QM_IOT_OTA_SUPPORT
#include "qm_iot_ota.h"
#endif

#if CONFIG_QM_IOT_DEVMNT_SUPPORT
#include "qm_iot_dev.h"
#endif

#if CONFIG_QM_IOT_DYNREG_SUPPORT
#include "qm_iot_dynreg.h"
#endif

#if CONFIG_QM_IOT_NTP_SUPPORT
#include "qm_time.h"
#include "qm_iot_ntp.h"
#endif 

#if CONFIG_QM_IOT_PUB_SUPPORT
#include "qm_iot_pub.h"
#include "qm_iot_shadow.h"
#define CONFIG_QM_IOT_PUB_DELAY_MS              (100)
#endif

#if CONFIG_QM_IOT_MQTT_SUPPORT
#include "qm_iot_mqtt.h"
#define QM_IOT_CORE_MQTT_PORT   (8883)
#define QM_IOT_CORE_MQTT_CLIENT_ID_PARAMS       "%ld_%s" //pid_sn

#endif

typedef struct 
{

    qm_iot_cloud_info_t cloud_info;

    char ca_crt[CONFIG_QM_IOT_CRT_MAX_LEN + 1];
    char client_crt[CONFIG_QM_IOT_CRT_MAX_LEN + 1];
    char client_key[CONFIG_QM_IOT_CRT_MAX_LEN + 1];
    char ota_signatrure[CONFIG_QM_IOT_OTA_CRT_MAX_LEN + 1];
}qm_iot_core_crt_handle_t;

typedef struct 
{
    void *api_handle;
    void *dynreg_handle;
    void *prov_handle;
    
    int status;
    int conn_router;
    int old_event;
    qm_work_t post_work;
    qm_iot_reset_info_t reset_info;

#if CONFIG_QM_IOT_CORE_WEATHER_SUPPORT
    void *weather_handle;
#endif

#if CONFIG_QM_IOT_OTA_SUPPORT
    void *ota_handle;
#endif
    
#if CONFIG_QM_IOT_PUB_SUPPORT
    void *pub_handle;
#endif

#if CONFIG_QM_IOT_MQTT_SUPPORT
    void *mqtt_handle;
#endif


#if CONFIG_QM_IOT_DEVMNT_SUPPORT
    void *dev_handle;
#endif

#if CONFIG_QM_IOT_AUDIO_URI_SUPPORT
    void *voice_handle;
#endif

#if CONFIG_QM_IOT_NTP_SUPPORT
    int8_t timezone;
    void *ntp_handle;
#endif

#if CONFIG_QM_IOT_DYNREG_SUPPORT
    uint8_t dynreg_enable;
    char *product_secret;
#endif
    
    qm_work_t core_req;

    qm_task_t core_task;
    qm_queue_t core_queue;
    qm_iot_core_crt_handle_t crt_handle;
#if CONFIG_QM_IOT_NETWORK_TYPE_WIFI
    qm_iot_wifi_info_t wifi_info;
#endif

#if CONFIG_QM_IOT_NETWORK_TYPE_4G
    int get_lbs;
    int conn_mqtt;
    qm_modem_t modem;
    qm_mutex_t _4G_lock;
#endif

}qm_iot_core_handle_t;

extern int32_t qm_iot_getopt(void *handle, qm_iot_option_t option, void **data);

static void qm_iot_core_conn_destory(qm_iot_core_handle_t *core_handle);

int32_t qm_iot_core_send(void *handle, uint32_t did, qm_iot_core_msg_type_t msg_type, qm_spec_property_operation_t *property_operation)
{
    int ret = QM_EOK;
#if CONFIG_QM_IOT_PUB_SUPPORT
    qm_iot_pub_msg_t pub_msg = {0};
#endif
    qm_iot_core_handle_t *core_handle = (qm_iot_core_handle_t *)handle;
    if(core_handle == NULL){
        return -QM_EINVAL;
    }

    switch (msg_type)
    {
    
    #if CONFIG_QM_IOT_CORE_WEATHER_SUPPORT
        case QM_IOT_CORE_MSG_TYPE_WEATHER_REQUEST:
            if(core_handle->weather_handle == NULL) {
                ret = -QM_EINVAL;
                break;
            }
            ret = qm_iot_weather_request(core_handle->weather_handle);

        break;
    #endif

    #if CONFIG_QM_IOT_PUB_SUPPORT
        case QM_IOT_CORE_MSG_TYPE_AWS_REPORT:
            if(property_operation == NULL || core_handle->pub_handle == NULL){
                ret = -QM_EINVAL;
                break;
            }
            pub_msg.did = did;
            pub_msg.type =  QM_IOT_PUBMSG_SHADOW_UPDATE;
            pub_msg.data.update.property_operation = property_operation;
            ret = qm_iot_pub_send(core_handle->pub_handle, &pub_msg, 0);
        break;

        case QM_IOT_CORE_MSG_TYPE_AWS_REPORT_FORCE:
            if(property_operation == NULL || core_handle->pub_handle == NULL ){
                ret = -QM_EINVAL;
                break;
            }
            pub_msg.did = did;
            pub_msg.type =  QM_IOT_PUBMSG_SHADOW_UPDATE;
            pub_msg.data.update.property_operation = property_operation;
            ret = qm_iot_pub_send(core_handle->pub_handle, &pub_msg, 1);
        break;

    #if CONFIG_QM_IOT_SPEC_SUPPORT

        case QM_IOT_CORE_MSG_TYPE_GET_REQ:
            if(property_operation == NULL || core_handle->pub_handle == NULL){
                ret = -QM_EINVAL;
                break;
            }
            pub_msg.did = did;
            pub_msg.type =  QM_IOT_PUBMSG_GET_REQ;
            pub_msg.data.update.property_operation = property_operation;
            ret = qm_iot_pub_send(core_handle->pub_handle, &pub_msg, 0);
        break;

        case QM_IOT_CORE_MSG_TYPE_GET_RSP:
            if(property_operation == NULL || core_handle->pub_handle == NULL){
                ret = -QM_EINVAL;
                break;
            }
            pub_msg.did = did;
            pub_msg.type =  QM_IOT_PUBMSG_GET_RSP;
            pub_msg.data.update.property_operation = property_operation;
            ret = qm_iot_pub_send(core_handle->pub_handle, &pub_msg, 0);
        break;

        case QM_IOT_CORE_MSG_TYPE_REPORT:
            if(property_operation == NULL || core_handle->pub_handle == NULL){
                ret = -QM_EINVAL;
                break;
            }
            pub_msg.did = did;
            pub_msg.type =  QM_IOT_PUBMSG_UPDATE;
            pub_msg.data.update.property_operation = property_operation;
            ret = qm_iot_pub_send(core_handle->pub_handle, &pub_msg, 0);
        break;

        case QM_IOT_CORE_MSG_TYPE_REPORT_FORCE:
            if(property_operation == NULL || core_handle->pub_handle == NULL ){
                ret = -QM_EINVAL;
                break;
            }
            pub_msg.did = did;
            pub_msg.type =  QM_IOT_PUBMSG_UPDATE;
            pub_msg.data.update.property_operation = property_operation;
            ret = qm_iot_pub_send(core_handle->pub_handle, &pub_msg, 1);
        break;
    #endif

    #endif

        default:
            ret = -QM_EINVAL;
        break;
    }

    return ret;
}

static void qm_iot_event_post(qm_iot_core_handle_t *core_handle, qm_iot_event_t *event)
{
    void *userdata = NULL;
    qm_iot_event_handler_t event_handler = NULL;

    if(core_handle == NULL){
        return;
    }

    qm_iot_getopt(core_handle->api_handle, QM_IOT_OPT_USERDATA, &userdata);
    qm_iot_getopt(core_handle->api_handle, QM_IOT_OPT_EVENT_HANDLER, (void **)&event_handler);
        
    if(event_handler){
        event_handler(core_handle->api_handle, event, userdata);
    }
}

#if CONFIG_QM_IOT_NETWORK_TYPE_WIFI

static void qm_iot_prov_delay_conn_wifi(void *arg)
{
    qm_iot_core_handle_t *core_handle = (qm_iot_core_handle_t *)arg;
    qm_iot_core_event_notify(core_handle, QM_IOT_EVENT_CONNING_LOCAL);
}

static void qm_comm_conn_handler(qm_input_event_t *input_event, void *arg)
{
    qm_iot_core_handle_t *core_handle = (qm_iot_core_handle_t *)arg;
    qm_wifi_event_info_t *event_info = (qm_wifi_event_info_t*)input_event->value;

    if(core_handle == NULL){
        return ;
    }

    switch(input_event->sub_event){

        case QM_WIFI_EVENT_STA_START:
            QM_LOGD(LOG_TAG, "connect to the AP");
            qm_wifi_connect();
        break;

        case QM_WIFI_EVENT_STA_STOP:

        break;

        case QM_WIFI_EVENT_STA_CONNECTED:
            QM_LOGD(LOG_TAG, "connected to the AP");
            core_handle->conn_router = QM_TRUE;
        break;

        case QM_WIFI_EVENT_STA_GOT_IP:
            QM_LOGD(LOG_TAG, "sta got ip");

            QM_LOGD(LOG_TAG, "ip:"      QM_IPSTR, QM_IP2STR(&event_info->got_ip.ip_info.ip));
            QM_LOGD(LOG_TAG, "netmask:" QM_IPSTR, QM_IP2STR(&event_info->got_ip.ip_info.netmask));
            QM_LOGD(LOG_TAG, "gw:"      QM_IPSTR, QM_IP2STR(&event_info->got_ip.ip_info.gw));

            qm_iot_wifi_ip_info_set(&event_info->got_ip.ip_info);
            core_handle->status = QM_IOT_EVENT_LOCAL_CONN;
            qm_iot_core_event_notify(core_handle, QM_IOT_EVENT_LOCAL_CONN);
        break;

        case QM_WIFI_EVENT_STA_LOST_IP:

        break;

        case QM_WIFI_EVENT_STA_DISCONNECTED:

            QM_LOGD(LOG_TAG, "sta disconnected reason: %d,retry to connect to the AP", event_info->sta_disconnected.reason);

        #if CONFIG_QM_IOT_MQTT_SUPPORT
            qm_iot_core_conn_destory(core_handle);
        #endif

            core_handle->conn_router = QM_FALSE;
            core_handle->status = QM_IOT_EVENT_LOCAL_DISCONN;
            qm_iot_core_event_notify(core_handle, QM_IOT_EVENT_LOCAL_DISCONN);
            qm_wifi_connect();
        break;

        default:
        break;
    }
}
#endif

#if CONFIG_QM_IOT_NETWORK_TYPE_4G
static void event_handler(qm_modem_event_t event, void *data, int len, void *arg)
{
    qm_iot_core_handle_t *core_handle = (qm_iot_core_handle_t *)arg;
#if CONFIG_QM_IOT_DEV_INFO_SUPPORT
    qm_modem_lbs_t *modem_lbs = NULL;
    char imsi[CONFIG_QM_IOT_4G_IMSI_MAX_LEN] = {0};
#endif    
    switch(event){

        case QM_MODEM_EVENT_CONN:
            QM_LOGD(LOG_TAG, "modem event conn");
            
        #if CONFIG_QM_IOT_DEV_INFO_SUPPORT
            qm_modem_get_lbs(core_handle->modem);
        #endif

            core_handle->status = QM_IOT_EVENT_LOCAL_CONN;
            qm_iot_core_event_notify(core_handle, QM_IOT_EVENT_LOCAL_CONN);
        break;

        case QM_MODEM_EVENT_DISCONN:
            QM_LOGD(LOG_TAG, "modem event disconn");

        #if CONFIG_QM_IOT_MQTT_SUPPORT
            qm_iot_core_conn_destory(core_handle);
        #endif 
            core_handle->get_lbs = QM_FALSE;
            core_handle->conn_mqtt = QM_FALSE;
            core_handle->status = QM_IOT_EVENT_LOCAL_DISCONN;
            qm_iot_core_event_notify(core_handle, QM_IOT_EVENT_LOCAL_DISCONN);
        break;
    #if CONFIG_QM_IOT_DEV_INFO_SUPPORT
        case QM_MODEM_EVENT_LBS:
            QM_LOGD(LOG_TAG, "modem event lbs");
            modem_lbs = (qm_modem_lbs_t *)data;
            qm_iot_4g_lbs_info_set(modem_lbs);
            qm_mutex_lock(&core_handle->_4G_lock, CONFIG_QM_IOT_4G_LOCK_TIMEOUT_MS);
            core_handle->get_lbs = QM_TRUE;
            if(core_handle->dev_handle && core_handle->conn_mqtt)
            {
                qm_iot_dev_send_devinfo(core_handle->dev_handle);
            }
            qm_mutex_unlock(&core_handle->_4G_lock);
        break;
    #endif
        case QM_MODEM_EVENT_SIMCARD_CONN:
            QM_LOGD(LOG_TAG, "modem event simcard conn");
        break;

        case QM_MODEM_EVENT_SIMCARD_DISCONN:
            QM_LOGD(LOG_TAG, "modem event simcard disconn");
        break;

        case QM_MODEM_EVENT_RESTART:
            QM_LOGD(LOG_TAG, "modem event restart");
        break;

        case QM_MODEM_EVENT_RESTART_DONE:
            QM_LOGD(LOG_TAG, "modem event restart done");
        break;

        default:
            break;
    }
}
#endif

#ifdef CONFIG_QM_IOT_PROV_SUPPORT
static void qm_iot_prov_recv_handler(void *handle, const qm_iot_prov_recv_t *packet, void *userdata)
{
    size_t ssid_len = 0;
    size_t password_len = 0;
    qm_wifi_config_t config = {0};
    qm_iot_core_handle_t *core_handle = (qm_iot_core_handle_t *)userdata;
    if(core_handle == NULL || handle == NULL || packet == NULL){
        return;
    }

#if CONFIG_QM_IOT_NTP_SUPPORT
    core_handle->timezone = packet->timezone;
#endif

#if CONFIG_QM_IOT_DYNREG_SUPPORT
    size_t dynreg_host_len = strlen(packet->dynreg_host);
    if(dynreg_host_len < CONFIG_QM_IOT_DYNREG_MAX_LEN) {
        qm_iot_dynreg_host_set(packet->dynreg_host, dynreg_host_len);
    }
#endif

    qm_cancel_delayed_action(&core_handle->post_work);

    // 添加边界检查防止缓冲区溢出
    ssid_len = strlen(packet->ssid);
    password_len = strlen(packet->password);
    if(ssid_len >= sizeof(config.sta.ssid) || password_len >= sizeof(config.sta.password)) {
        return;
    }

    config.sta.ssid_len = ssid_len;
    memcpy(config.sta.ssid, packet->ssid, ssid_len);
    config.sta.password_len = password_len;
    memcpy(config.sta.password, packet->password, password_len);

    memset(&core_handle->wifi_info, 0, sizeof(qm_iot_wifi_info_t));
    if(ssid_len < sizeof(core_handle->wifi_info.ssid) && password_len < sizeof(core_handle->wifi_info.pwd)) {
        memcpy(core_handle->wifi_info.ssid, config.sta.ssid, ssid_len);
        memcpy(core_handle->wifi_info.pwd, config.sta.password, password_len);
    }
    
    qm_iot_prov_deinit(&core_handle->prov_handle);
    qm_iot_wifi_info_set(&core_handle->wifi_info);

#if CONFIG_QM_IOT_AUDIO_URI_SUPPORT
    if(packet->audio_uri){

        qm_iot_audio_uri_set(packet->audio_uri, strlen(packet->audio_uri));
    }
#endif

    qm_post_delayed_action(&core_handle->post_work, qm_iot_prov_delay_conn_wifi, core_handle, 1000);
}

static void qm_iot_prov_timeout_notify(void *arg)
{
    qm_iot_core_handle_t *core_handle = (qm_iot_core_handle_t *)arg;

    if(core_handle->prov_handle){
        qm_iot_prov_deinit(&core_handle->prov_handle);
    }
    qm_iot_core_event_notify(core_handle, QM_IOT_EVENT_CONFIG_TIMEOUT);
}

static void qm_iot_core_prov_process(qm_iot_core_handle_t *core_handle)
{
    if(core_handle == NULL){
        return ;
    }

    core_handle->prov_handle = qm_iot_prov_init();
    if( core_handle->prov_handle == NULL){
        QM_LOGE(LOG_TAG, "PROVISION CREATE FAILED!!");
        return ;
    }

    qm_iot_prov_setopt(core_handle->prov_handle, QM_IOT_PROVDOPT_RECV_HANDLER, qm_iot_prov_recv_handler);
    qm_iot_prov_setopt(core_handle->prov_handle, QM_IOT_PROVDOPT_USERDATA, (void *)core_handle);

    qm_iot_prov_start(core_handle->prov_handle);
#if CONFIG_QM_IOT_CORE_PROV_TIMEOUT_S >= 0
    qm_post_delayed_action(&core_handle->post_work, qm_iot_prov_timeout_notify, (void *)core_handle , CONFIG_QM_IOT_CORE_PROV_TIMEOUT_S * 1000);
#endif
}
#endif

static void qm_iot_core_conn_process(qm_iot_core_handle_t *core_handle)
{
#if CONFIG_QM_IOT_NETWORK_TYPE_WIFI 
    qm_wifi_config_t config = {0};

    // 添加边界检查
    size_t ssid_len = strlen(core_handle->wifi_info.ssid);
    size_t password_len = strlen(core_handle->wifi_info.pwd);
    
    if(ssid_len >= sizeof(config.sta.ssid) || password_len >= sizeof(config.sta.password)) {
        return;
    }

    config.sta.ssid_len = ssid_len;
    memcpy(config.sta.ssid, core_handle->wifi_info.ssid, ssid_len);
    config.sta.password_len = password_len;
    memcpy(config.sta.password, core_handle->wifi_info.pwd, password_len);

    QM_LOGD(LOG_TAG, "CONN WIFI [%s]:[%s]", config.sta.ssid, config.sta.password);

    qm_wifi_init();
    qm_wifi_set_mode(QM_WIFI_MODE_STA);
    qm_wifi_set_config(QM_WIFI_IF_STA, &config);
    qm_wifi_start();
#endif
}

#if CONFIG_QM_IOT_DYNREG_SUPPORT
static void qm_iot_dynreg_recv_handler(void *handle, const qm_iot_dynreg_recv_t *packet, void *userdata)
{
    qm_iot_core_handle_t *core_handle = (qm_iot_core_handle_t *)userdata;
    if(core_handle == NULL || handle == NULL || packet == NULL){
        return;
    }

    if(packet->type == QM_IOT_DYNREGRECV_STATUS_CODE){
        QM_LOGE(LOG_TAG, "Dynreg recv errcode:%d", packet->data.status_code.code);
        return ;
    }

#if CONFIG_QM_IOT_MQTT_SUPPORT
    memset(&core_handle->crt_handle, 0, sizeof(qm_iot_core_crt_handle_t));

    core_handle->crt_handle.cloud_info.port = QM_IOT_CORE_MQTT_PORT;

    memcpy(core_handle->crt_handle.cloud_info.host, packet->data.device_info.mqtt_host, packet->data.device_info.mqtt_host_len);
    memcpy(core_handle->crt_handle.ca_crt, packet->data.device_info.server_cert, packet->data.device_info.server_cert_len);
    memcpy(core_handle->crt_handle.client_crt, packet->data.device_info.client_crt, packet->data.device_info.client_cert_len);
    memcpy(core_handle->crt_handle.client_key, packet->data.device_info.client_private_key, packet->data.device_info.client_privkey_len);
    memcpy(core_handle->crt_handle.ota_signatrure, packet->data.device_info.public_key, packet->data.device_info.public_key_len);
    
    qm_iot_server_crt_set(core_handle->crt_handle.ca_crt, strlen(core_handle->crt_handle.ca_crt));
    qm_iot_client_crt_set(core_handle->crt_handle.client_crt, strlen(core_handle->crt_handle.client_crt));
    qm_iot_client_key_set(core_handle->crt_handle.client_key, strlen(core_handle->crt_handle.client_key));
    qm_iot_ota_public_key_set(core_handle->crt_handle.ota_signatrure, strlen(core_handle->crt_handle.ota_signatrure));

    qm_iot_cloud_info_set(&core_handle->crt_handle.cloud_info);
#endif

    qm_iot_did_set(packet->data.device_info.did);

    core_handle->status = QM_IOT_EVENT_CONNING_CLOUD;
    qm_iot_core_event_notify(core_handle, QM_IOT_EVENT_CONNING_CLOUD);

    qm_iot_dynreg_deinit(&core_handle->dynreg_handle);
}

static void qm_iot_core_dynreg_process(qm_iot_core_handle_t *core_handle)
{

    uint16_t port = CONFIG_QM_IOT_CORE_DYNREG_PORT;
    uint32_t retery = CONFIG_QM_IOT_CORE_DYNREG_RETRY_COUNT;
    uint32_t timeout = CONFIG_QM_IOT_CORE_DYNREG_RECVTIMEOUT;

    core_handle->dynreg_handle = qm_iot_dynreg_init();
    if(core_handle->dynreg_handle == NULL){
        QM_LOGE(LOG_TAG, "DYNREG CREATE FAILED!!");
        return ;
    }


    qm_iot_dynreg_setopt(core_handle->dynreg_handle, QM_IOT_DYNREGOPT_RECV_TIMEOUT_MS, &timeout);
    qm_iot_dynreg_setopt(core_handle->dynreg_handle, QM_IOT_DYNREGOPT_HOST, qm_iot_dynreg_host_get());
    qm_iot_dynreg_setopt(core_handle->dynreg_handle, QM_IOT_DYNREGOPT_PORT, &port);
    qm_iot_dynreg_setopt(core_handle->dynreg_handle, QM_IOT_DYNREGOPT_PRODUCT_SECRET, core_handle->product_secret);
    qm_iot_dynreg_setopt(core_handle->dynreg_handle, QM_IOT_DYNREGOPT_DEVICE_SN, qm_iot_sn_get());
    qm_iot_dynreg_setopt(core_handle->dynreg_handle, QM_IOT_DYNREGOPT_MAX_RETRY_NUM, &retery);

    qm_iot_dynreg_setopt(core_handle->dynreg_handle, QM_IOT_DYNREGOPT_RECV_HANDLER, qm_iot_dynreg_recv_handler);
    qm_iot_dynreg_setopt(core_handle->dynreg_handle, QM_IOT_DYNREGOPT_USERDATA, (void *)core_handle);

    qm_iot_dynreg_start(core_handle->dynreg_handle);

}
#endif

#if CONFIG_QM_IOT_PUB_SUPPORT
static void qm_iot_pub_recv_handler(void *handle, qm_iot_pub_recv_t *recv, void *userdata)
{
    qm_iot_recv_t iot_recv = {0};
    void *user_data = NULL;
    qm_iot_recv_handler_t recv_handler = NULL;
    qm_iot_core_handle_t *core_handle = (qm_iot_core_handle_t *)userdata;
    if(core_handle == NULL || handle == NULL || recv == NULL){
        return;
    }

    iot_recv.did = recv->did;
    qm_iot_getopt(core_handle->api_handle, QM_IOT_OPT_USERDATA, &user_data);
    qm_iot_getopt(core_handle->api_handle, QM_IOT_OPT_RECV_HANDLER, (void **)&recv_handler);

    switch (recv->type)
    {
        case QM_IOT_PUBRECV_SHADOW_SET:
            iot_recv.type = QM_IOT_RECV_TYPE_SET;
            iot_recv.siid = recv->data.set.spec_property->siid;
            iot_recv.piid = recv->data.set.spec_property->piid;
            iot_recv.data.set.property = recv->data.set.spec_property;
            if(recv_handler){
                recv_handler(core_handle->api_handle, &iot_recv, user_data);
            }
        break;
        
        case QM_IOT_PUBRECV_PROPERTY_SET:
            iot_recv.type = QM_IOT_RECV_TYPE_PROPERTY_SET;
            iot_recv.did = recv->data.prop_set.spec_property->did;
            iot_recv.siid = recv->data.prop_set.spec_property->siid;
            iot_recv.piid = recv->data.prop_set.spec_property->piid;
            iot_recv.data.set.property = recv->data.prop_set.spec_property;
            if(recv_handler){
                recv_handler(core_handle->api_handle, &iot_recv, user_data);
            }
        break;

        case QM_IOT_PUBRECV_PROPERTY_GET_RSP:
            iot_recv.type = QM_IOT_RECV_TYPE_PROPERTY_GET_RSP;
            iot_recv.did = recv->data.prop_set.spec_property->did;
            iot_recv.siid = recv->data.prop_get.spec_property->siid;
            iot_recv.piid = recv->data.prop_get.spec_property->piid;
            iot_recv.data.get_rsp.property = recv->data.prop_get.spec_property;
            if(recv_handler){
                recv_handler(core_handle->api_handle, &iot_recv, user_data);
            }
        break;

        case QM_IOT_PUBRECV_PROPERTY_GET_REQ:
            iot_recv.type = QM_IOT_RECV_TYPE_PROPERTY_GET_REQ;
            iot_recv.did = recv->data.prop_set.spec_property->did;
            iot_recv.siid = recv->data.prop_get.spec_property->siid;
            iot_recv.piid = recv->data.prop_get.spec_property->piid;
            iot_recv.data.get_req.property = recv->data.prop_get.spec_property;
            if(recv_handler){
                recv_handler(core_handle->api_handle, &iot_recv, user_data);
            }
        break;

        default:
        break;
    }
}

static void qm_iot_core_pub_process(qm_iot_core_handle_t *core_handle)
{
    uint32_t delay_pub = CONFIG_QM_IOT_PUB_DELAY_MS;
    if(core_handle == NULL){
        return;
    }

    if(core_handle->pub_handle == NULL){
        core_handle->pub_handle = qm_iot_pub_init();
        if(core_handle->pub_handle == NULL){
            return;
        }
    }

    qm_iot_pub_setopt(core_handle->pub_handle, QM_IOT_PUBOPT_USERDATA, core_handle);
    qm_iot_pub_setopt(core_handle->pub_handle, QM_IOT_PUBOPT_DELAY_TIMEOUT, &delay_pub);
    qm_iot_pub_setopt(core_handle->pub_handle, QM_IOT_PUBOPT_MQTT_HANDLE, core_handle->mqtt_handle);
    qm_iot_pub_setopt(core_handle->pub_handle, QM_IOT_PUBOPT_RECV_HANDLER, qm_iot_pub_recv_handler);

}
#endif

#if CONFIG_QM_IOT_OTA_SUPPORT
static void qm_iot_ota_event_handler(void *handle, const qm_iot_ota_event_t *event, void *userdata)
{
    qm_iot_event_t iot_event = {0};
    qm_iot_ota_event_t *ota_event = (qm_iot_ota_event_t *)event;
    qm_iot_core_handle_t *core_handle = (qm_iot_core_handle_t *)userdata;
    if(core_handle == NULL || handle == NULL){
        return;
    }

    switch (event->type)
    {
        case QM_IOT_OTA_EVENT_READY:
            iot_event.type = QM_IOT_EVENT_OTA_READY;
            qm_iot_event_post(core_handle, &iot_event);
            ota_event->data.ready.result = (qm_iot_ota_req_result_t)iot_event.data.ota_ready.is_busy;
        break;

        case QM_IOT_OTA_EVENT_START:
            iot_event.type = QM_IOT_EVENT_OTA_START;
            qm_iot_event_post(core_handle, &iot_event);
        break;
        
        case QM_IOT_OTA_EVENT_DATA_INSTALLING:
            iot_event.type = QM_IOT_EVENT_OTA_INSTALLING;
            qm_iot_event_post(core_handle, &iot_event);
        break;
        
        case QM_IOT_OTA_EVENT_PROGRESS_UPADTE:
            iot_event.type = QM_IOT_EVENT_OTA_PROGRESS_UPADTE;
            iot_event.data.ota_progress_update.progress_details = 
                            event->data.progress_update.progress_details;
            qm_iot_event_post(core_handle, &iot_event);
        break;

        case QM_IOT_OTA_EVENT_SUCCESS:
    #if CONFIG_QM_IOT_NETWORK_TYPE_WIFI
            if(core_handle && core_handle->conn_router){
                qm_wifi_disconnect();
            }
    #endif
            iot_event.type = QM_IOT_EVENT_OTA_SUCCESS;
            qm_iot_event_post(core_handle, &iot_event);
        break;

        case QM_IOT_OTA_EVENT_FAILED:
            iot_event.type = QM_IOT_EVENT_OTA_FAILED;
            qm_iot_event_post(core_handle, &iot_event);
        break;
        
        case QM_IOT_OTA_EVENT_CANCEL:
            iot_event.type = QM_IOT_EVENT_OTA_CANCEL;
            qm_iot_event_post(core_handle, &iot_event);
        break;

        default:
            break;
    }
}

static void qm_iot_core_ota_process(qm_iot_core_handle_t *core_handle)
{
    if(core_handle == NULL){
        return;
    }

    if(core_handle->ota_handle == NULL){
        core_handle->ota_handle = qm_iot_ota_init();
        if(core_handle->ota_handle == NULL){
            return;
        }
    }

    qm_iot_ota_setopt(core_handle->ota_handle, QM_IOT_OTAOPT_MQTT_HANDLE, core_handle->mqtt_handle);
    qm_iot_ota_setopt(core_handle->ota_handle, QM_IOT_OTAOPT_USERDATA, core_handle);
    qm_iot_ota_setopt(core_handle->ota_handle, QM_IOT_OTAOPT_SINGATURE, core_handle->crt_handle.ota_signatrure);
    qm_iot_ota_setopt(core_handle->ota_handle, QM_IOT_OTAOPT_EVENT_HANDLER, qm_iot_ota_event_handler);

}
#endif



static void iot_req_action(void *arg)
{
    qm_iot_core_handle_t *core_handle = (qm_iot_core_handle_t *)arg;
    if(core_handle == NULL){
        return ;
    }

    qm_cancel_delayed_action(&core_handle->core_req); 

#if CONFIG_QM_IOT_NTP_SUPPORT
    if(core_handle->ntp_handle){
        qm_iot_ntp_request(core_handle->ntp_handle);
    }
#endif

#if CONFIG_QM_IOT_DEV_INFO_SUPPORT
    if(core_handle->dev_handle)
    {
    #if CONFIG_QM_IOT_NETWORK_TYPE_4G
        if(core_handle->get_lbs && core_handle->conn_mqtt)
    #endif
            qm_iot_dev_send_devinfo(core_handle->dev_handle);
    }
#endif

    qm_post_delayed_action(&core_handle->core_req, iot_req_action, (void *)core_handle, CONFIG_QM_IOT_REQ_INTERVAL * 1000);
}

#if CONFIG_QM_IOT_NTP_SUPPORT
static void qm_iot_ntp_recv_handler(void *handle, const qm_iot_ntp_recv_t *packet, void *userdata)
{
    qm_iot_event_t iot_event = {0};
    struct qm_timeval timeval = {0};
    uint32_t timestamp = (uint32_t)packet->data.local_time.timestamp;
    qm_iot_core_handle_t *core_handle = (qm_iot_core_handle_t *)userdata;
    if(packet == NULL || userdata == NULL){
        return ;
    }

    timeval.tv_sec = packet->data.local_time.timestamp;
    qm_settimeofday(&timeval, NULL);
    QM_LOGD(LOG_TAG, "timezone %u, timestamp %u", core_handle->timezone, timestamp);
}

static void qm_iot_core_ntp_process(qm_iot_core_handle_t *core_handle)
{
    uint32_t deinit_timeout = CONFIG_QM_IOT_NTP_DEINIT_TIMEOUT;
    if(core_handle == NULL){
        return;
    }

    if(core_handle->ntp_handle == NULL){
        core_handle->ntp_handle = qm_iot_ntp_init();
        if(core_handle->ntp_handle == NULL){
            return;
        }
    }

    core_handle->timezone = CONFIG_QM_IOT_NTP_TIMEZONE;
    qm_iot_ntp_timezone_get(&core_handle->timezone);

    qm_iot_ntp_setopt(core_handle->ntp_handle, QM_IOT_NTPOPT_MQTT_HANDLE, core_handle->mqtt_handle);
    qm_iot_ntp_setopt(core_handle->ntp_handle, QM_IOT_NTPOPT_TIME_ZONE, &core_handle->timezone);
    qm_iot_ntp_setopt(core_handle->ntp_handle, QM_IOT_NTPOPT_RECV_HANDLER, qm_iot_ntp_recv_handler);
    qm_iot_ntp_setopt(core_handle->ntp_handle, QM_IOT_NTPOPT_USERDATA, core_handle);
    qm_iot_ntp_setopt(core_handle->ntp_handle, QM_IOT_NTPOPT_DEINIT_TIMEOUT_MS, &deinit_timeout);

}
#endif

static void qm_iot_mqtt_reset_notify(void *arg)
{
#if CONFIG_QM_IOT_NETWORK_TYPE_WIFI
    qm_iot_core_handle_t *core_handle = (qm_iot_core_handle_t *)arg;
    
    if(core_handle && core_handle->conn_router){
        qm_wifi_disconnect();
    }
    
#endif
    qm_reboot();
}

#if CONFIG_QM_IOT_DEVMNT_SUPPORT

static void qm_iot_dev_recv_handler(void *handle, const qm_iot_dev_recv_t *packet, void *userdata)
{
    qm_iot_event_t  event = {0};
    qm_iot_core_handle_t *core_handle = (qm_iot_core_handle_t *)userdata;
    if(packet == NULL || userdata == NULL){
        return ;
    }

    qm_cancel_delayed_action(&core_handle->post_work);
    switch (packet->type)
    {
        case QM_IOT_DEVRECV_RESET_REPLY:
            QM_LOGD(LOG_TAG, "QM_IOT_DEVRECV_RESET_REPLY");
            
            if(!core_handle->reset_info.is_reset){
                core_handle->reset_info.is_reset = QM_TRUE;
                qm_iot_reset_info_set(&core_handle->reset_info);
            }

            qm_post_delayed_action(&core_handle->post_work, qm_iot_mqtt_reset_notify, core_handle , 0);
        break;

        case QM_IOT_DEVRECV_UNBIND_NOTIFY:
            QM_LOGD(LOG_TAG, "QM_IOT_DEVRECV_UNBIND_NOTIFY");

            if(!core_handle->reset_info.is_reset){
                core_handle->reset_info.is_reset = QM_TRUE;
                qm_iot_reset_info_set(&core_handle->reset_info);
            }

            event.type = QM_IOT_EVENT_RESET;
            qm_iot_event_post(core_handle, &event);

            qm_post_delayed_action(&core_handle->post_work, qm_iot_mqtt_reset_notify,core_handle ,CONFIG_QM_IOT_RESET_DELAY_MS);
        break;

        case QM_IOT_DEVRECV_BIND_NOTIFY:
            QM_LOGD(LOG_TAG, "QM_IOT_DEVRECV_BIND_NOTIFY");
            if(core_handle->reset_info.is_reset){
                core_handle->reset_info.is_reset = QM_FALSE;
                qm_iot_reset_info_set(&core_handle->reset_info);
            }
            event.type = QM_IOT_EVENT_BIND_NOTIFY;
            qm_iot_event_post(core_handle, &event);
        break;

        default:
            break;
    }
}

static void qm_iot_unbind_notify(qm_iot_core_handle_t *core_handle)
{
    if(core_handle == NULL){
        return ;
    }

    if(!core_handle->reset_info.is_reset){
        core_handle->reset_info.is_reset = QM_TRUE;
        qm_iot_reset_info_set(&core_handle->reset_info);
    }

    qm_cancel_delayed_action(&core_handle->post_work);

    if(core_handle->mqtt_handle == NULL || core_handle->dev_handle == NULL){
        qm_post_delayed_action(&core_handle->post_work, qm_iot_mqtt_reset_notify,core_handle ,CONFIG_QM_IOT_RESET_DELAY_MS);
        return;
    }
    qm_iot_dev_reset_request(core_handle->dev_handle);
    qm_post_delayed_action(&core_handle->post_work, qm_iot_mqtt_reset_notify,core_handle ,CONFIG_QM_IOT_RESET_FORCE_MS);
}

static void qm_iot_core_dev_process(qm_iot_core_handle_t *core_handle)
{
    if(core_handle == NULL){
        return;
    }

    if(core_handle->dev_handle == NULL){
        core_handle->dev_handle = qm_iot_dev_init();
        if(core_handle->dev_handle == NULL){
            return;
        }
    }

    qm_iot_dev_setopt(core_handle->dev_handle, QM_IOT_DEVOPT_MQTT_HANDLE, core_handle->mqtt_handle);
    qm_iot_dev_setopt(core_handle->dev_handle, QM_IOT_DEVOPT_RECV_HANDLER, qm_iot_dev_recv_handler);
    qm_iot_dev_setopt(core_handle->dev_handle, QM_IOT_DEVOPT_USERDATA, core_handle);
}
#else
static void qm_iot_unbind_notify(qm_iot_core_handle_t *core_handle)
{
    if(core_handle == NULL){
        return;
    }

    qm_cancel_delayed_action(&core_handle->post_work);
    
    if(!core_handle->reset_info.is_reset){
        core_handle->reset_info.is_reset = QM_TRUE;
        qm_iot_reset_info_set(&core_handle->reset_info);
    }

    qm_post_delayed_action(&core_handle->post_work, qm_iot_mqtt_reset_notify, core_handle ,CONFIG_QM_IOT_RESET_DELAY_MS);
}
#endif

#if CONFIG_QM_IOT_NETWORK_TYPE_4G
static void qm_iot_mqtt_stop_notify(void *arg)
{
    qm_iot_core_handle_t *core_handle = (qm_iot_core_handle_t *)arg;
    if(core_handle == NULL){
        return ;
    }
    qm_iot_core_stop(core_handle);
}
#endif

static void qm_iot_core_process_start(qm_iot_core_handle_t *core_handle)
{

#if CONFIG_QM_IOT_NETWORK_TYPE_4G
    qm_iot_reset_info_t reset_info = {0};
#endif

#if CONFIG_QM_IOT_MQTT_SUPPORT
    qm_iot_mqtt_pre_sub_start(core_handle->mqtt_handle);
#endif

    qm_cancel_delayed_action(&core_handle->core_req);  
#if CONFIG_QM_IOT_NTP_SUPPORT
    if(core_handle->ntp_handle){
        qm_iot_ntp_request(core_handle->ntp_handle);
    }
#endif

#if CONFIG_QM_IOT_DEV_INFO_SUPPORT
    if(core_handle->dev_handle)
    {
    #if CONFIG_QM_IOT_NETWORK_TYPE_4G
        if(core_handle->get_lbs)
    #endif
            qm_iot_dev_send_devinfo(core_handle->dev_handle);
    }
#endif
    qm_post_delayed_action(&core_handle->core_req, iot_req_action, (void *)core_handle, CONFIG_QM_IOT_REQ_INTERVAL * 1000);

#if CONFIG_QM_IOT_OTA_SUPPORT
    qm_iot_ota_start(core_handle->ota_handle);
#endif


#if CONFIG_QM_IOT_NETWORK_TYPE_4G
    qm_iot_reset_info_get(&reset_info);
    if(reset_info.is_reset == QM_TRUE){
        qm_cancel_delayed_action(&core_handle->post_work);
    #if CONFIG_QM_IOT_CORE_PROV_TIMEOUT_S >= 0
        qm_post_delayed_action(&core_handle->post_work, qm_iot_mqtt_stop_notify, (void *)core_handle, CONFIG_QM_IOT_CORE_PROV_TIMEOUT_S * 1000);
    #endif
    }
#endif

}

#if CONFIG_QM_IOT_CORE_WEATHER_SUPPORT

static void qm_iot_weather_recv_handler(void *handle, const qm_iot_weather_recv_t *packet, void *userdata)
{
    qm_iot_event_t iot_event = {0};
    qm_iot_core_handle_t *core_handle = (qm_iot_core_handle_t *)userdata;
    if(packet == NULL || userdata == NULL){
        return ;
    }

    iot_event.type = QM_IOT_EVENT_WEATHER_NOTIFY;

    iot_event.data.weather_notify.weather_type = packet->weather_type;
    iot_event.data.weather_notify.temp = packet->temp;
    iot_event.data.weather_notify.humidity = packet->humidity;
    iot_event.data.weather_notify.city_id = packet->city_id;

    iot_event.data.weather_notify.condition_len = packet->condition_len;
    iot_event.data.weather_notify.country_name_len = packet->country_name_len;
    iot_event.data.weather_notify.pname_len = packet->pname_len;
    iot_event.data.weather_notify.secondaryname_len = packet->secondaryname_len;
    iot_event.data.weather_notify.name_len = packet->name_len;
    iot_event.data.weather_notify.sunset_len = packet->sunset_len;
    iot_event.data.weather_notify.sunrise_len = packet->sunrise_len;

    iot_event.data.weather_notify.condition = packet->condition;
    iot_event.data.weather_notify.country_name = packet->country_name;
    iot_event.data.weather_notify.pname = packet->pname;
    iot_event.data.weather_notify.secondaryname = packet->secondaryname;
    iot_event.data.weather_notify.name = packet->name;
    iot_event.data.weather_notify.sunset = packet->sunset;
    iot_event.data.weather_notify.sunrise = packet->sunrise;

    qm_iot_event_post(core_handle, &iot_event);
}


static void qm_iot_core_weather_process(qm_iot_core_handle_t *core_handle)
{
    uint32_t delay_pub = CONFIG_QM_IOT_WEATHER_DELAY_MS;
    if(core_handle == NULL){
        return;
    }

    if(core_handle->weather_handle == NULL){
        core_handle->weather_handle = qm_iot_weather_init();
        if(core_handle->weather_handle == NULL){
            return;
        }
    }

    qm_iot_weather_setopt(core_handle->weather_handle, QM_IOT_WEATHEROPT_USERDATA, core_handle);
    qm_iot_weather_setopt(core_handle->weather_handle, QM_IOT_WEATHEROPT_DEINIT_TIMEOUT_MS, &delay_pub);
    qm_iot_weather_setopt(core_handle->weather_handle, QM_IOT_WEATHEROPT_MQTT_HANDLE, core_handle->mqtt_handle);
    qm_iot_weather_setopt(core_handle->weather_handle, QM_IOT_WEATHEROPT_RECV_HANDLER, qm_iot_weather_recv_handler);
}
#endif

#if CONFIG_QM_IOT_MQTT_SUPPORT
static void qm_iot_mqtt_event_handler(void *handle, const qm_iot_mqtt_event_t *event, void *userdata)
{
    qm_iot_event_t iot_event = {0};
    qm_iot_reset_info_t reset_info = {0};
    qm_iot_core_handle_t *core_handle = (qm_iot_core_handle_t *)userdata;
    if(core_handle == NULL || handle == NULL || event == NULL){
        return;
    }
    switch (event->type)
    {
        case QM_IOT_MQTTEVT_CONNECT:
        #if CONFIG_QM_IOT_PUB_SUPPORT
            qm_iot_core_pub_process(core_handle);
        #endif
            qm_iot_core_ota_process(core_handle);
        
        #if CONFIG_QM_IOT_DEVMNT_SUPPORT
            qm_iot_core_dev_process(core_handle);
        #endif

        #if CONFIG_QM_IOT_NTP_SUPPORT
            qm_iot_core_ntp_process(core_handle);
        #endif

        #if CONFIG_QM_IOT_CORE_WEATHER_SUPPORT
            qm_iot_core_weather_process(core_handle);
        #endif
        #if CONFIG_QM_IOT_NETWORK_TYPE_4G
            qm_mutex_lock(&core_handle->_4G_lock, CONFIG_QM_IOT_4G_LOCK_TIMEOUT_MS);
        #endif

            qm_iot_core_process_start(core_handle);
        
        #if CONFIG_QM_IOT_NETWORK_TYPE_4G
            core_handle->conn_mqtt = QM_TRUE;
            qm_mutex_unlock(&core_handle->_4G_lock);
            qm_iot_reset_info_get(&reset_info);
            if (reset_info.is_reset){
                return ;
            }
        #endif
        
            iot_event.type = QM_IOT_EVENT_CLOUD_CONN;
            qm_iot_event_post(core_handle, &iot_event);
        break;

        case QM_IOT_MQTTEVT_DISCONNECT:
        #if CONFIG_QM_IOT_NETWORK_TYPE_4G
            core_handle->conn_mqtt = QM_FALSE;
        #endif
            qm_cancel_delayed_action(&core_handle->core_req);  
            iot_event.type = QM_IOT_EVENT_CLOUD_DISCONN;
            qm_iot_event_post(core_handle, &iot_event);
        break;

        case QM_IOT_MQTTEVT_RECONNECT:
            qm_iot_core_process_start(core_handle);
            
            iot_event.type = QM_IOT_EVENT_CLOUD_CONN;
            qm_iot_event_post(core_handle, &iot_event);
        break;
        default:
        break;
    }
}

static void qm_iot_core_conn_destory(qm_iot_core_handle_t *core_handle)
{
    if(core_handle->mqtt_handle == NULL){
        return;
    }
#if CONFIG_QM_IOT_OTA_SUPPORT
    if(core_handle->ota_handle == NULL){
        return;
    }
#endif
    qm_cancel_delayed_action(&core_handle->core_req);  
#if CONFIG_QM_IOT_OTA_SUPPORT
    qm_iot_ota_stop(core_handle->ota_handle);
#endif
    qm_iot_mqtt_deinit(&core_handle->mqtt_handle);
}

static void qm_iot_core_conn_mqtt_process(qm_iot_core_handle_t *core_handle)
{
    qm_iot_mqtt_network_cred_t net_cred = {0};
    uint32_t pid;   
    uint8_t clear_session = QM_TRUE;
    uint32_t repub_timeout_ms = CONFIG_QM_IOT_CORE_MQTT_REPUB_MS;

    uint16_t keeplive = CONFIG_QM_IOT_CORE_MQTT_KEEPLIVE_S;
    char str_did[CONFIG_QM_IOT_CORE_MQTT_CID_LEN + 1] = {0};

    if(core_handle->mqtt_handle == NULL){
        core_handle->mqtt_handle = qm_iot_mqtt_init();
        if(core_handle->mqtt_handle == NULL){
            return;
        }
#if CONFIG_QM_IOT_NETWORK_TYPE_WIFI 
    if(core_handle->reset_info.is_reset){
        core_handle->reset_info.is_reset = QM_FALSE;
        qm_iot_reset_info_set(&core_handle->reset_info);
    }
#endif
    }

    net_cred.server_crt = core_handle->crt_handle.ca_crt;
    net_cred.client_crt = core_handle->crt_handle.client_crt;
    net_cred.client_privkey = core_handle->crt_handle.client_key;

    pid = qm_iot_pid_get();
    snprintf(str_did, CONFIG_QM_IOT_CORE_MQTT_CID_LEN, QM_IOT_CORE_MQTT_CLIENT_ID_PARAMS, pid, qm_iot_sn_get());

    qm_iot_mqtt_setopt(core_handle->mqtt_handle, QM_IOT_MQTTOPT_HOST, core_handle->crt_handle.cloud_info.host);
    qm_iot_mqtt_setopt(core_handle->mqtt_handle, QM_IOT_MQTTOPT_PORT, &core_handle->crt_handle.cloud_info.port);
    qm_iot_mqtt_setopt(core_handle->mqtt_handle, QM_IOT_MQTTOPT_CLIENTID, str_did);
    qm_iot_mqtt_setopt(core_handle->mqtt_handle, QM_IOT_MQTTOPT_KEEPALIVE_SEC, &keeplive);
    qm_iot_mqtt_setopt(core_handle->mqtt_handle, QM_IOT_MQTTOPT_NETWORK_CRED, &net_cred);
    qm_iot_mqtt_setopt(core_handle->mqtt_handle, QM_IOT_MQTTOPT_REPUB_TIMEOUT_MS, &repub_timeout_ms);
    qm_iot_mqtt_setopt(core_handle->mqtt_handle, QM_IOT_MQTTOPT_USERDATA, (void *)core_handle);
    qm_iot_mqtt_setopt(core_handle->mqtt_handle, QM_IOT_MQTTOPT_CLEAN_SESSION, &clear_session);
    qm_iot_mqtt_setopt(core_handle->mqtt_handle, QM_IOT_MQTTOPT_EVENT_HANDLER, qm_iot_mqtt_event_handler);

    qm_iot_mqtt_process(core_handle->mqtt_handle);
}
#endif

static void qm_iot_core_task(void *arg)
{
    int ret = QM_EOK;
    unsigned int event_size = 0;
    qm_iot_event_t  event = {0};
    qm_iot_event_type_t event_notify = QM_IOT_EVENT_NONE;
    qm_iot_core_handle_t *core_handle = (qm_iot_core_handle_t *)arg;
    
    while (1)
    {
        ret = qm_queue_recv(&core_handle->core_queue, &event_notify, &event_size, QM_WAIT_FOREVER);
        if(ret != QM_EOK){
            continue;
        }
        event.type = event_notify;

        switch (event_notify)
        {
            case QM_IOT_EVENT_CONFIG_TIMEOUT:
                qm_iot_event_post(core_handle, &event);
            break;

        #ifdef CONFIG_QM_IOT_PROV_SUPPORT
            case QM_IOT_EVENT_CONFIG:
                qm_iot_core_prov_process(core_handle);
                qm_iot_event_post(core_handle, &event);
            break;
        #endif
            case QM_IOT_EVENT_LOCAL_DISCONN:
                qm_iot_event_post(core_handle, &event);
            break;

            case QM_IOT_EVENT_CONNING_LOCAL:
                qm_iot_event_post(core_handle, &event);
                qm_iot_core_conn_process(core_handle);
            break;

            case QM_IOT_EVENT_LOCAL_CONN:
                qm_iot_event_post(core_handle, &event);
            #if CONFIG_QM_IOT_DYNREG_SUPPORT
                if(core_handle->dynreg_enable){
                    qm_iot_core_dynreg_process(core_handle);
                }else{
            #endif
            #if CONFIG_QM_IOT_NETWORK_TYPE_WIFI 
                    core_handle->status = QM_IOT_EVENT_CONNING_CLOUD;
            #endif
                    qm_iot_core_event_notify(core_handle, QM_IOT_EVENT_CONNING_CLOUD);
            #if CONFIG_QM_IOT_DYNREG_SUPPORT
                }
            #endif
            break;

            case QM_IOT_EVENT_CONNING_CLOUD:

            #if CONFIG_QM_IOT_DYNREG_SUPPORT
                core_handle->dynreg_enable = QM_FALSE;
            #endif

            #if CONFIG_QM_IOT_MQTT_SUPPORT
                qm_iot_core_conn_mqtt_process(core_handle);
            #else
                #if CONFIG_QM_IOT_NETWORK_TYPE_WIFI
                event.type = QM_IOT_EVENT_CLOUD_CONN;
                if(!core_handle->reset_info.is_reset){
                    core_handle->reset_info.is_reset = QM_TRUE;
                    qm_iot_reset_info_set(&core_handle->reset_info);
                }
                #endif
            #endif
                qm_iot_event_post(core_handle, &event);
            
            break;

            default:
            break;
        }
    }
}

int32_t qm_iot_core_stop(void *handle)
{
    qm_iot_core_handle_t *core_handle = (qm_iot_core_handle_t *)handle;
    if(core_handle == NULL){
        return -QM_EINVAL;
    }
#if CONFIG_QM_IOT_NETWORK_TYPE_WIFI 
    qm_event_unregister(QM_EVENT_WIFI, qm_comm_conn_handler, (void *)core_handle);
#endif
#if CONFIG_QM_IOT_MQTT_SUPPORT
    qm_iot_core_conn_destory(core_handle);
#endif

    return QM_EOK;
}

int32_t qm_iot_core_start(void *handle)
{
    uint32_t len = 0;
    int ret = QM_EOK;

#if CONFIG_QM_IOT_NETWORK_TYPE_WIFI
    // todo : 根据实际参数更改
    uint8_t mac[6] = {0};
    char str_mac[12 + 1] = {0};
#endif

    char *sn = NULL;
    uint32_t did;
    qm_iot_core_handle_t *core_handle = (qm_iot_core_handle_t *)handle;
    if(core_handle == NULL){
        return -QM_EINVAL;
    }
#if CONFIG_QM_IOT_DYNREG_SUPPORT && CONFIG_QM_IOT_MQTT_SUPPORT
    memset(&core_handle->crt_handle, 0, sizeof(qm_iot_core_crt_handle_t));
    core_handle->crt_handle.cloud_info.port = QM_IOT_CORE_MQTT_PORT;
#endif

#if CONFIG_QM_IOT_NETWORK_TYPE_WIFI 

    sn = qm_iot_sn_get();
    if(sn[0] == '\0'){
        qm_ble_gap_get_local_addr(mac);
        qm_snprintf(str_mac, 12 + 1, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        qm_iot_sn_set(str_mac, strlen(str_mac));
    }
    
    ret = qm_iot_wifi_info_get(&core_handle->wifi_info);
    if(ret != QM_EOK){
        QM_LOGW(LOG_TAG, "WIFI GET FAILED!!");
        core_handle->reset_info.is_reset = QM_TRUE;
        goto __next;
    }
#endif
    
    qm_iot_reset_info_get(&core_handle->reset_info);
    if(core_handle->reset_info.is_reset == QM_TRUE){
        QM_LOGD(LOG_TAG, "DEVICE RESET!!");
        goto __next;
    }

#if CONFIG_QM_IOT_DYNREG_SUPPORT && CONFIG_QM_IOT_MQTT_SUPPORT
   did = qm_iot_did_get();
    if(did <= 0){
        QM_LOGW(LOG_TAG, "DID GET FAILED!!");
        core_handle->reset_info.is_reset = QM_TRUE;
        goto __next;
    }

    ret = qm_iot_cloud_info_get(&core_handle->crt_handle.cloud_info);
    if(ret != QM_EOK){
        QM_LOGW(LOG_TAG, "CLOUD GET FAILED!!");
        core_handle->reset_info.is_reset = QM_TRUE;
        goto __next;
    }

    len = CONFIG_QM_IOT_CRT_MAX_LEN;
    ret = qm_iot_server_crt_get(core_handle->crt_handle.ca_crt, &len);
    if(ret != QM_EOK || len == 0){
        QM_LOGW(LOG_TAG, "ROOT CA GET FAILED!!");
        core_handle->reset_info.is_reset = QM_TRUE;

        goto __next;
    }

    len = CONFIG_QM_IOT_CRT_MAX_LEN;
    ret = qm_iot_client_crt_get(core_handle->crt_handle.client_crt, &len);
    if(ret != QM_EOK || len == 0){
        QM_LOGW(LOG_TAG, "CLIENT CA GET FAILED!!");

        core_handle->reset_info.is_reset = QM_TRUE;
        goto __next;
    }

    len = CONFIG_QM_IOT_CRT_MAX_LEN;
    ret = qm_iot_client_key_get(core_handle->crt_handle.client_key, &len);
    if(ret != QM_EOK || len == 0){
        QM_LOGW(LOG_TAG, "CLIENT KEY GET FAILED!!");

        core_handle->reset_info.is_reset = QM_TRUE;
        goto __next;
    }


    len = CONFIG_QM_IOT_OTA_CRT_MAX_LEN;
    ret = qm_iot_ota_public_key_get(core_handle->crt_handle.ota_signatrure, &len);
    if(ret != QM_EOK || len == 0){
        QM_LOGW(LOG_TAG, "PUBLIC KEY GET FAILED!!");

        core_handle->reset_info.is_reset = QM_TRUE;
        goto __next;
    }
#endif

__next:
#if CONFIG_QM_IOT_DYNREG_SUPPORT
    core_handle->product_secret = qm_iot_prodtuct_sercet_get();
#endif

#if CONFIG_QM_IOT_NETWORK_TYPE_WIFI 
    qm_event_register(QM_EVENT_WIFI, qm_comm_conn_handler, (void *)core_handle);
#endif

    if(core_handle->reset_info.is_reset){
    #if CONFIG_QM_IOT_DYNREG_SUPPORT
        core_handle->dynreg_enable = QM_TRUE;
    #endif
        core_handle->status = QM_IOT_EVENT_CONFIG;
    }

    qm_iot_core_event_notify(core_handle, core_handle->status);

#if CONFIG_QM_IOT_NETWORK_TYPE_4G 
    qm_modem_config_t cfg = {
        .rx_buffer_size = 5*1024,
        .tx_buffer_size = 5*1024,
        .channel = QM_MODEM_CHANNLE_USB,
        .event_handler = event_handler,
        .arg = core_handle,
    };

    core_handle->modem = qm_modem_init(&cfg);
    qm_modem_start_ppp(core_handle->modem, 20000);
#endif

    return QM_EOK;
}

void *qm_iot_core_init(void *api_handle)
{
    int ret = QM_EOK;
    qm_iot_core_handle_t *core_handle = (qm_iot_core_handle_t *)qm_malloc(sizeof(qm_iot_core_handle_t));
    if(core_handle == NULL){
        return NULL;
    }
    memset(core_handle, 0, sizeof(qm_iot_core_handle_t));

    core_handle->api_handle = api_handle;
    core_handle->status = QM_IOT_EVENT_CONNING_LOCAL;
    ret = qm_queue_new(&core_handle->core_queue, CONFIG_QM_IOT_CORE_TASK_QUEUE_NUM, sizeof(qm_iot_event_type_t));
    if(ret != QM_EOK){
        goto __error;
    }
    
#if CONFIG_QM_IOT_NETWORK_TYPE_4G
    ret = qm_mutex_new(&core_handle->_4G_lock);
    if(ret != QM_EOK){
        goto __error;
    }
#endif

    ret = qm_task_new(&core_handle->core_task, "core_task", qm_iot_core_task, (void *)core_handle, CONFIG_QM_IOT_CORE_TASK_SIZE, CONFIG_QM_IOT_CORE_TASK_PRO);
    if(ret != QM_EOK){
        goto __error;
    }
    return (void *)core_handle;

__error:

    qm_queue_free(&core_handle->core_queue);
#if CONFIG_QM_IOT_NETWORK_TYPE_4G
    qm_mutex_free(&core_handle->_4G_lock);
#endif

    if(core_handle){
        qm_free(core_handle);
    }
    return NULL;
}

int32_t qm_iot_core_deinit(void **handle)
{
    return QM_EOK;
}

int32_t qm_iot_core_event_notify(void *handle, qm_iot_event_type_t event)
{
    static qm_iot_event_type_t old_event = QM_IOT_EVENT_NONE;
    qm_iot_core_handle_t *core_handle = (qm_iot_core_handle_t *)handle;
    if(core_handle == NULL || old_event == event){
        return -QM_EINVAL;
    }

    old_event = event;
    
    switch (event)
    {
        case QM_IOT_EVENT_RESET:
            qm_iot_unbind_notify(core_handle);
        break;
        
        default:
            core_handle->status = event;
            qm_queue_send(&core_handle->core_queue, &event , sizeof(qm_iot_event_type_t));
        break;        
    }
    return QM_EOK;
}