#include "qm.h"
#include "qm_kv.h"
#include "qm_iot_config.h"

#define LOG_TAG "IOT_CONFIG"

#ifndef CONFIG_QM_IOT_KV_HEADER
#define CONFIG_QM_IOT_KV_HEADER   "cloud/"
#endif

#define RESET_INFO_KEY          (CONFIG_QM_IOT_KV_HEADER"reset")
#define DID_INFO_KEY            (CONFIG_QM_IOT_KV_HEADER"did")
#define CLOUD_INFO_KEY          (CONFIG_QM_IOT_KV_HEADER"cloud")
#define WIFI_INFO_KEY           (CONFIG_QM_IOT_KV_HEADER"wifi")
#define SERVER_CRT_INFO_KEY     (CONFIG_QM_IOT_KV_HEADER"s_ca")
#define CLIENT_CRT_INFO_KEY     (CONFIG_QM_IOT_KV_HEADER"c_ca")
#define CLIENT_KEY_INFO_KEY     (CONFIG_QM_IOT_KV_HEADER"c_key")
#define PUBLIC_KEY_INFO_KEY     (CONFIG_QM_IOT_KV_HEADER"p_key")
#define AUDIO_URI_KEY           (CONFIG_QM_IOT_KV_HEADER"audio")
#define AGORA_URI_KEY           (CONFIG_QM_IOT_KV_HEADER"agora")
#define SN_KEY                  (CONFIG_QM_IOT_KV_HEADER"sn")

#if CONFIG_QM_IOT_NTP_SUPPORT
#define TIMEZONE_KEY_INFO_KEY   (CONFIG_QM_IOT_KV_HEADER"timezone")
#endif

typedef struct 
{
    uint32_t did;

    uint32_t pid;

    uint32_t version;
    
    uint32_t mcu_version;

    char *prodtuct_sercet;

    int8_t timezone;

    char sn[CONFIG_QM_IOT_SN_MAX_LEN + 1];
    char dynreg_host[CONFIG_QM_IOT_DYNREG_MAX_LEN];

#if CONFIG_QM_IOT_AUTHCODE_SUPPORT
    char authcode[CONFIG_QM_IOT_AUTHCODE_MAX_LEN];
#endif

#if CONFIG_QM_IOT_NETWORK_TYPE_WIFI
    qm_ip_info_t ip_info;
#endif

#if CONFIG_QM_IOT_NETWORK_TYPE_4G
    qm_modem_lbs_t lbs_info;
    char imsi[CONFIG_QM_IOT_4G_IMSI_MAX_LEN];
#endif

}qm_iot_config_handle_t;

static qm_iot_config_handle_t g_config_handle = {0};

#if CONFIG_QM_IOT_NTP_SUPPORT
int qm_iot_ntp_timezone_set(int8_t timezone)
{
    return qm_kv_set(TIMEZONE_KEY_INFO_KEY, &timezone, sizeof(int8_t), 1);
}

int qm_iot_ntp_timezone_get(int8_t *timezone)
{
    int len = sizeof(int8_t);
    return qm_kv_get(TIMEZONE_KEY_INFO_KEY, timezone, &len);
}
#endif

char *qm_iot_dynreg_host_get(void)
{
    return g_config_handle.dynreg_host;
}

int qm_iot_dynreg_host_set(char *host, uint32_t len)
{
    if(host == NULL || len == 0) {
        return -QM_EINVAL;
    }
    
    if(len >= CONFIG_QM_IOT_DYNREG_MAX_LEN) {
        return -QM_EINVAL;
    }
    
    memset(g_config_handle.dynreg_host, 0, CONFIG_QM_IOT_DYNREG_MAX_LEN);
    memcpy(g_config_handle.dynreg_host, host, len);
    return QM_EOK;
}


int qm_iot_sn_set(char *sn, uint32_t len)
{
    if(sn == NULL || len == 0) {
        return -QM_EINVAL;
    }
    
    if(len >= CONFIG_QM_IOT_SN_MAX_LEN) {
        return -QM_EINVAL;
    }
    
#if CONFIG_QM_IOT_SN_KV_SUPPORT
    qm_kv_set(SN_KEY, sn, len, 1);
#endif
    memset(g_config_handle.sn, 0, CONFIG_QM_IOT_SN_MAX_LEN + 1);
    memcpy(g_config_handle.sn, sn, len);

    return QM_EOK;
}

char *qm_iot_sn_get(void)
{
#if CONFIG_QM_IOT_SN_KV_SUPPORT
    int len = CONFIG_QM_IOT_SN_MAX_LEN;
    if(g_config_handle.sn[0] == '\0'){
        qm_kv_get(SN_KEY, g_config_handle.sn, &len);
    }
#endif
    return g_config_handle.sn;
}


int32_t qm_iot_prodtuct_sercet_set(char *prodtuct_sercet, int len)
{
    if(prodtuct_sercet == NULL || len <= 0) {
        return -QM_EINVAL;
    }
    
    // 先清理旧的密钥
    if(g_config_handle.prodtuct_sercet){
        qm_free(g_config_handle.prodtuct_sercet);
        g_config_handle.prodtuct_sercet = NULL;
    }
    g_config_handle.prodtuct_sercet = (char *)qm_malloc(len + 1);
    if(g_config_handle.prodtuct_sercet == NULL){
        return -QM_ENOMEM;
    }
    
    memset(g_config_handle.prodtuct_sercet, 0, len + 1);
    memcpy(g_config_handle.prodtuct_sercet, prodtuct_sercet, len);

    return QM_EOK;
}

char *qm_iot_prodtuct_sercet_get(void)
{
    return g_config_handle.prodtuct_sercet;
}

int32_t qm_iot_version_set(uint32_t version)
{
    g_config_handle.version = version;
    return QM_EOK;
}

uint32_t qm_iot_version_get(void)
{
    return g_config_handle.version;
}


int32_t qm_iot_mcu_version_set(uint32_t version)
{
    g_config_handle.mcu_version = version;
    return QM_EOK;
}

uint32_t qm_iot_mcu_version_get(void)
{
    return g_config_handle.mcu_version;
}

int32_t qm_iot_reset_info_set(qm_iot_reset_info_t *reset_info)
{
    return qm_kv_set(RESET_INFO_KEY, reset_info, sizeof(qm_iot_reset_info_t), 1);
}

int32_t qm_iot_reset_info_get(qm_iot_reset_info_t *reset_info)
{
    int len = sizeof(qm_iot_reset_info_t);
    return qm_kv_get(RESET_INFO_KEY, reset_info, &len);
}

int32_t qm_iot_pid_set(uint32_t pid)
{
    g_config_handle.pid = pid;
    return QM_EOK;
}

uint32_t qm_iot_pid_get(void)
{
    return g_config_handle.pid;
}

int32_t qm_iot_did_set(uint32_t did)
{
    g_config_handle.did = did;
    return qm_kv_set(DID_INFO_KEY, &did, sizeof(uint32_t), 1);
}

uint32_t qm_iot_did_get(void)
{
    int len = sizeof(int);
    
    if(!g_config_handle.did){
        qm_kv_get(DID_INFO_KEY, &g_config_handle.did, &len);
    }

    return g_config_handle.did;
}

int32_t qm_iot_cloud_info_set(qm_iot_cloud_info_t *cloud_info)
{
    return qm_kv_set(CLOUD_INFO_KEY, cloud_info, sizeof(qm_iot_cloud_info_t), 1);
}

int32_t qm_iot_cloud_info_get(qm_iot_cloud_info_t *cloud_info)
{
    int len = sizeof(qm_iot_cloud_info_t);
    return qm_kv_get(CLOUD_INFO_KEY, cloud_info, (int *)&len);
}

#if CONFIG_QM_IOT_NETWORK_TYPE_WIFI
int32_t qm_iot_wifi_info_set(qm_iot_wifi_info_t *wifi_info)
{
    return qm_kv_set(WIFI_INFO_KEY, wifi_info, sizeof(qm_iot_wifi_info_t), 1);
}

int32_t qm_iot_wifi_info_get(qm_iot_wifi_info_t *wifi_info)
{
    int len = sizeof(qm_iot_wifi_info_t);
    return qm_kv_get(WIFI_INFO_KEY, wifi_info, (int *)&len);
}

int32_t qm_iot_wifi_ip_info_set(qm_ip_info_t *ip_info)
{
    if(ip_info == NULL){
        return -QM_EINVAL;
    }

    memcpy(&g_config_handle.ip_info, ip_info, sizeof(qm_ip_info_t));

    return QM_EOK;
}

int32_t qm_iot_wifi_ip_info_get(qm_ip_info_t *ip_info)
{
    if(ip_info == NULL){
        return -QM_EINVAL;
    }

    memcpy(ip_info, &g_config_handle.ip_info, sizeof(qm_ip_info_t));
    return QM_EOK;
}
#endif

int32_t qm_iot_server_crt_set(char *server_crt, uint32_t len)
{
    if(server_crt == NULL || len == 0) {
        return -QM_EINVAL;
    }
    
    if(len > CONFIG_QM_IOT_CRT_MAX_LEN) {
        return -QM_EINVAL;
    }
    
    return qm_kv_set(SERVER_CRT_INFO_KEY, server_crt, len, 1);
}

int32_t qm_iot_server_crt_get(char *server_crt, uint32_t *len)
{
    return qm_kv_get(SERVER_CRT_INFO_KEY, server_crt, (int *)len);
}

int32_t qm_iot_client_crt_set(char *client_crt, uint32_t len)
{
    if(client_crt == NULL || len == 0) {
        return -QM_EINVAL;
    }
    
    if(len > CONFIG_QM_IOT_CRT_MAX_LEN) {
        return -QM_EINVAL;
    }
    
    return qm_kv_set(CLIENT_CRT_INFO_KEY, client_crt, len, 1);
}

int32_t qm_iot_client_crt_get(char *client_crt, uint32_t *len)
{
    return qm_kv_get(CLIENT_CRT_INFO_KEY, client_crt, (int *)len);
}

int32_t qm_iot_client_key_set(char *client_key, uint32_t len)
{
    if(client_key == NULL || len == 0) {
        return -QM_EINVAL;
    }
    
    if(len > CONFIG_QM_IOT_CRT_MAX_LEN) {
        return -QM_EINVAL;
    }
    
    return qm_kv_set(CLIENT_KEY_INFO_KEY, client_key, len, 1);
}

int32_t qm_iot_client_key_get(char *client_key, uint32_t *len)
{
    return qm_kv_get(CLIENT_KEY_INFO_KEY, client_key, (int *)len);
}

int32_t qm_iot_ota_public_key_set(char *public_key, uint32_t len)
{
    if(public_key == NULL || len == 0) {
        return -QM_EINVAL;
    }
    
    if(len > CONFIG_QM_IOT_OTA_CRT_MAX_LEN) {
        return -QM_EINVAL;
    }
    
    return qm_kv_set(PUBLIC_KEY_INFO_KEY, public_key, len, 1);
}

int32_t qm_iot_ota_public_key_get(char *public_key, uint32_t *len)
{
    return qm_kv_get(PUBLIC_KEY_INFO_KEY, public_key, (int *)len);
}

#if CONFIG_QM_IOT_AUDIO_URI_SUPPORT
int qm_iot_audio_uri_set(char *uri, uint32_t len)
{
    return qm_kv_set(AUDIO_URI_KEY, uri, len, 1);
}

int32_t qm_iot_audio_uri_get(char *uri, uint32_t *len)
{
    return qm_kv_get(AUDIO_URI_KEY, uri, (int *)len);
}

int qm_iot_agora_uri_set(char *uri, uint32_t len)
{
    return qm_kv_set(AGORA_URI_KEY, uri, len, 1);
}

int32_t qm_iot_agora_uri_get(char *uri, uint32_t *len)
{
    return qm_kv_get(AGORA_URI_KEY, uri, (int *)len);
}
#endif

#if CONFIG_QM_IOT_AUTHCODE_SUPPORT
char *qm_iot_authcode_get(void)
{
    if(g_config_handle.authcode[0] == '\0'){
        return NULL;
    }

    return g_config_handle.authcode;
}

int qm_iot_authcode_set(char *auth_code, uint32_t len)
{
    memcpy(g_config_handle.authcode, auth_code, len);
    return QM_EOK;
}

#endif

#if CONFIG_QM_IOT_NETWORK_TYPE_4G

int32_t qm_iot_4g_lbs_info_set(qm_modem_lbs_t *lbs_info)
{
    if(lbs_info == NULL){
        return -QM_EINVAL;
    }    

    memcpy(&g_config_handle.lbs_info, lbs_info, sizeof(qm_modem_lbs_t));

    return QM_EOK;
}

int32_t qm_iot_4g_lbs_info_get(qm_modem_lbs_t *lbs_info)
{
    if(lbs_info == NULL){
        return -QM_EINVAL;
    }    

    memcpy(lbs_info, &g_config_handle.lbs_info, sizeof(qm_modem_lbs_t));

    return QM_EOK;
}


char *qm_iot_4g_imsi_info_get(void)
{
    return g_config_handle.imsi;
}

int32_t qm_iot_4g_imsi_info_set(char imsi[CONFIG_QM_IOT_4G_IMSI_MAX_LEN])
{
    memset(g_config_handle.imsi, 0, CONFIG_QM_IOT_4G_IMSI_MAX_LEN);
    memcpy(g_config_handle.imsi, imsi, strlen(imsi));

    return QM_EOK;
}


#endif