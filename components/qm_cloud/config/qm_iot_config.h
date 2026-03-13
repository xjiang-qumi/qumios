#ifndef __QM_IOT_CONFIG_H__
#define __QM_IOT_CONFIG_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include "qm.h"

#if CONFIG_QM_IOT_NETWORK_TYPE_WIFI
#include "qm_wifi.h"
#endif

#if CONFIG_QM_IOT_NETWORK_TYPE_4G
#include "qm_modem.h"
#endif

#define CONFIG_QM_IOT_SDK_VESRION               ("1.0.0")

#ifndef CONFIG_QM_IOT_SPEC_SUPPORT
#define CONFIG_QM_IOT_SPEC_SUPPORT              0
#endif

#ifndef CONFIG_QM_IOT_AUTHCODE_SUPPORT
#define CONFIG_QM_IOT_AUTHCODE_SUPPORT          0
#endif

#if CONFIG_QM_IOT_AUTHCODE_SUPPORT

#ifndef CONFIG_QM_IOT_AUTHCODE_MAX_LEN 
#define CONFIG_QM_IOT_AUTHCODE_MAX_LEN           32
#endif

#endif

#ifndef CONFIG_QM_IOT_SN_KV_SUPPORT 
#define CONFIG_QM_IOT_SN_KV_SUPPORT              (0)
#endif


#ifndef CONFIG_QM_IOT_CORE_WEATHER_SUPPORT
#define CONFIG_QM_IOT_CORE_WEATHER_SUPPORT       (0)
#endif

#ifndef CONFIG_QM_IOT_AUDIO_URI_SUPPORT
#define CONFIG_QM_IOT_AUDIO_URI_SUPPORT          0
#endif

#ifndef CONFIG_QM_IOT_OTA_SUPPORT
#define CONFIG_QM_IOT_OTA_SUPPORT                0
#endif

#ifndef CONFIG_QM_IOT_SLIENT_OTA_SUPPORT
#define CONFIG_QM_IOT_SLIENT_OTA_SUPPORT         1
#endif

#ifndef CONFIG_QM_IOT_BLE_CANCEL_DEINIT
#define CONFIG_QM_IOT_BLE_CANCEL_DEINIT          0
#endif

#ifndef CONFIG_QM_IOT_MQTT_SUPPORT
#define CONFIG_QM_IOT_MQTT_SUPPORT               0
#endif

#ifndef CONFIG_QM_IOT_PUB_SUPPORT
#define CONFIG_QM_IOT_PUB_SUPPORT               0
#endif

#ifndef CONFIG_QM_IOT_DEVMNT_SUPPORT
#define CONFIG_QM_IOT_DEVMNT_SUPPORT             0
#endif

#ifndef CONFIG_QM_IOT_DYNREG_SUPPORT
#define CONFIG_QM_IOT_DYNREG_SUPPORT             0
#endif

#ifndef CONFIG_QM_IOT_NTP_SUPPORT
#define CONFIG_QM_IOT_NTP_SUPPORT                0
#endif

#ifndef CONFIG_QM_IOT_NTP_TIMEZONE
#define CONFIG_QM_IOT_NTP_TIMEZONE               8
#endif

#ifndef CONFIG_QM_IOT_REQ_INTERVAL
#define CONFIG_QM_IOT_REQ_INTERVAL           (60 * 60)
#endif


#ifndef CONFIG_QM_IOT_NTP_DEINIT_TIMEOUT
#define CONFIG_QM_IOT_NTP_DEINIT_TIMEOUT         200
#endif

#ifndef CONFIG_QM_IOT_DID_MAX_LEN
#define CONFIG_QM_IOT_DID_MAX_LEN               16
#endif

#ifndef CONFIG_QM_IOT_PRODUCT_SECRET_MAX_LEN
#define CONFIG_QM_IOT_PRODUCT_SECRET_MAX_LEN    32
#endif

#ifndef CONFIG_QM_IOT_MQTT_HOST_MAX_LEN
#define CONFIG_QM_IOT_MQTT_HOST_MAX_LEN         64
#endif

#ifndef CONFIG_QM_IOT_SSID_MAX_LEN
#define CONFIG_QM_IOT_SSID_MAX_LEN              64
#endif

#ifndef CONFIG_QM_IOT_PWD_MAX_LEN
#define CONFIG_QM_IOT_PWD_MAX_LEN               64
#endif

#ifndef CONFIG_QM_IOT_AUDIO_URI_MAX_LEN
#define CONFIG_QM_IOT_AUDIO_URI_MAX_LEN         64
#endif

#ifndef CONFIG_QM_IOT_CRT_MAX_LEN 
#define CONFIG_QM_IOT_CRT_MAX_LEN               2048
#endif

#ifndef CONFIG_QM_IOT_OTA_CRT_MAX_LEN 
#define CONFIG_QM_IOT_OTA_CRT_MAX_LEN           384
#endif

#ifndef CONFIG_QM_IOT_DYNREG_MAX_LEN 
#define CONFIG_QM_IOT_DYNREG_MAX_LEN           384
#endif

#ifndef CONFIG_QM_IOT_SN_MAX_LEN 
#define CONFIG_QM_IOT_SN_MAX_LEN                32
#endif

#if CONFIG_QM_IOT_NETWORK_TYPE_4G

#ifndef CONFIG_QM_IOT_4G_IMSI_MAX_LEN 
#define CONFIG_QM_IOT_4G_IMSI_MAX_LEN           64
#endif

#ifndef CONFIG_QM_IOT_4G_LOCK_TIMEOUT_MS 
#define CONFIG_QM_IOT_4G_LOCK_TIMEOUT_MS        5000
#endif

#endif



typedef struct {
    enum{
        QM_IOT_RESET_UNPROV = 1,
        QM_IOT_RESET_PROV_WIFI,
    }is_reset;
}qm_iot_reset_info_t;

int32_t qm_iot_reset_info_set(qm_iot_reset_info_t *reset_info);
int32_t qm_iot_reset_info_get(qm_iot_reset_info_t *reset_info);

int32_t qm_iot_pid_set(uint32_t pid);
uint32_t qm_iot_pid_get(void);

int32_t qm_iot_did_set(uint32_t did);
uint32_t qm_iot_did_get(void);

typedef struct {
    char host[CONFIG_QM_IOT_MQTT_HOST_MAX_LEN + 1];
    uint16_t port;
}qm_iot_cloud_info_t;

int32_t qm_iot_cloud_info_set(qm_iot_cloud_info_t *cloud_info);
int32_t qm_iot_cloud_info_get(qm_iot_cloud_info_t *cloud_info);

#if CONFIG_QM_IOT_NETWORK_TYPE_WIFI

typedef struct {
    char ssid[CONFIG_QM_IOT_SSID_MAX_LEN + 1];
    char pwd[CONFIG_QM_IOT_PWD_MAX_LEN + 1];
}qm_iot_wifi_info_t;

int32_t qm_iot_wifi_info_set(qm_iot_wifi_info_t *wifi_info);
int32_t qm_iot_wifi_info_get(qm_iot_wifi_info_t *wifi_info);

int32_t qm_iot_wifi_ip_info_set(qm_ip_info_t *ip_info);
int32_t qm_iot_wifi_ip_info_get(qm_ip_info_t *ip_info);

#endif

#if CONFIG_QM_IOT_NETWORK_TYPE_4G

int32_t qm_iot_4g_lbs_info_set(qm_modem_lbs_t *lbs_info);
int32_t qm_iot_4g_lbs_info_get(qm_modem_lbs_t *lbs_info);

char *qm_iot_4g_imsi_info_get(void);
int32_t qm_iot_4g_imsi_info_set(char imsi[CONFIG_QM_IOT_4G_IMSI_MAX_LEN]);

#endif

int32_t qm_iot_server_crt_set(char *server_crt, uint32_t len);
int32_t qm_iot_server_crt_get(char *server_crt, uint32_t *len);

int32_t qm_iot_client_crt_set(char *client_crt, uint32_t len);
int32_t qm_iot_client_crt_get(char *client_crt, uint32_t *len);

int32_t qm_iot_client_key_set(char *client_key, uint32_t len);
int32_t qm_iot_client_key_get(char *client_key, uint32_t *len);

int32_t qm_iot_ota_public_key_set(char *public_key, uint32_t len);
int32_t qm_iot_ota_public_key_get(char *public_key, uint32_t *len);

int32_t qm_iot_version_set(uint32_t version);
uint32_t qm_iot_version_get(void);

int32_t qm_iot_mcu_version_set(uint32_t version);
uint32_t qm_iot_mcu_version_get(void);

int32_t qm_iot_prodtuct_sercet_set(char *prodtuct_sercet, int len);

char *qm_iot_prodtuct_sercet_get(void);

#if CONFIG_QM_IOT_NTP_SUPPORT
int qm_iot_ntp_timezone_set(int8_t timezone);
int qm_iot_ntp_timezone_get(int8_t *timezone);
#endif

#if CONFIG_QM_IOT_AUDIO_URI_SUPPORT
int qm_iot_audio_uri_set(char *uri, uint32_t len);
int32_t qm_iot_audio_uri_get(char *uri, uint32_t *len);
int qm_iot_agora_uri_set(char *uri, uint32_t len);
int32_t qm_iot_agora_uri_get(char *uri, uint32_t *len);
#endif

#if CONFIG_QM_IOT_AUTHCODE_SUPPORT
char *qm_iot_authcode_get(void);
int qm_iot_authcode_set(char *auth_code, uint32_t len);
#endif

char *qm_iot_sn_get(void);
int qm_iot_sn_set(char *sn, uint32_t len);

char *qm_iot_dynreg_host_get(void);
int qm_iot_dynreg_host_set(char *host, uint32_t len);

#if defined(__cplusplus)
}
#endif

#endif  /* __QM_IOT_CONFIG_H__ */