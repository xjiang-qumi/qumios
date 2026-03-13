#include "qm.h"
#include "qm_iot_dynreg.h"

#include "util_httpc.h"
#include "json_parser.h"
#include "backoff_algorithm.h"

#include "qm_utils_string.h"
#include "qm_utils_hmac.h"

#include "qm_iot_config.h"

#define LOG_TAG "DYNREG"

#define BACKOFF_BASE_TIME_MS    (1 * 1000)

#define DATA_MAX_STR_LEN        (128)
#define RSP_PAYLOAD_MAX_STR_LEN    (8 * 1024)

#define DEVICE_DYNREG_NONCE_MAX  ((1U << 31) - 1)

#if CONFIG_QM_IOT_NETWORK_TYPE_WIFI
#define SHADOW_DYNERG_REQ_FORMAT  "{\"sn\":\"%s\",\"pid\":\"%d\",\"timestamp\":\"%d\",\"signature\":\"%s\",\"nonce\":%d}"
#endif

#if CONFIG_QM_IOT_NETWORK_TYPE_4G
#if CONFIG_QM_IOT_DYNREG_4G_V1
#define SHADOW_DYNERG_REQ_FORMAT  "{\"pid\":%d,\"authCode\":\"%s\"}"
#else
#define SHADOW_DYNERG_REQ_FORMAT  "{\"pid\":%d,\"sn\":\"%s\",\"authCode\":\"%s\"}"
#endif
#endif

#define SHADOW_SN_KEY           "sn"
#define SHADOW_STATUS_KEY       "status"
#define SHADOW_RESULT_KEY       "result"
#define SHADOW_DID_KEY          "did"
#define SHADOW_HOST_KEY         "awsBrokerUrl"
#define AI_VOICE_HOST_KEY       "voiceUrl"
#define AGORA_HOST_KEY          "agoraGenTokenUrl"
#define SHADOW_CA_KEY           "awsRootPem"
#define SHADOW_CLIENT_CA_KEY    "certificatePem"
#define SHADOW_CLIENT_KEY       "privateKey"

#define SHADOW_PUBLIC_KEY       "otaPublicKey"

typedef struct 
{
    char *host;
    uint16_t port;

    int nonce;
    char *sn;
    uint32_t pid;
    char *product_secret;

}qm_iot_dynreg_req_t;

typedef struct 
{
    void *userdata;
    uint32_t retry_num;
    uint32_t recv_timeout;
    qm_iot_dynreg_req_t req_data;
    qm_iot_dynreg_recv_handler_t recv_handler;
}qm_iot_dynreg_handle_t;

static int qm_iot_dynreg_recv_handler(void *handle, char *buff, int len)
{
    int type;
    int code = 0;
    char *reuslt = NULL;
    int result_len = 0;
    char *sn = NULL;
    int sn_len = 0;
    char *voiceUrl = NULL;
    int voiceUrl_len = 0;
    char *agoraUrl = NULL;
    int agoraUrl_len = 0;
    char *value = NULL;
    int value_len = 0;

    char *did = NULL;
    int did_len = 0;
    qm_iot_dynreg_recv_t dynreg_recv = {0};
    qm_iot_dynreg_handle_t *dynreg_handle = (qm_iot_dynreg_handle_t *)handle;
    if(handle == NULL || buff == NULL || len == 0){
        return -QM_EINVAL;
    }

    //解析数据
    value = qm_json_get_value_by_name(buff, len, SHADOW_STATUS_KEY, &value_len, &type);
    if(value == NULL){
        return -QM_EINVAL;
    }
    code = int_str_to_num(value, value_len);
    if(code != QM_EOK){
        dynreg_recv.data.status_code.code = code;
        dynreg_recv.type = QM_IOT_DYNREGRECV_STATUS_CODE;
        goto __exit;
    }

    dynreg_recv.type = QM_IOT_DYNREGRECV_DEVICE_INFO;

    reuslt = qm_json_get_value_by_name(buff, len, SHADOW_RESULT_KEY, &result_len, &type);
    if(reuslt == NULL){
        QM_LOGE(LOG_TAG, "[reuslt] NULL!!");
        return -QM_EINVAL;
    }
    did = qm_json_get_value_by_name(reuslt, result_len, SHADOW_DID_KEY, (int *)&did_len, &type);
    if(did == NULL || !did_len){
        QM_LOGE(LOG_TAG, "[did] NULL!!");
        return -QM_EINVAL;
    }
    dynreg_recv.data.device_info.did = int_str_to_num(did, did_len);

    sn = qm_json_get_value_by_name(reuslt, result_len, SHADOW_SN_KEY, (int *)&sn_len, &type);
    if(sn != NULL && sn_len){
       qm_iot_sn_set(sn, sn_len);
    }

#if CONFIG_QM_IOT_AUDIO_URI_SUPPORT
    voiceUrl = qm_json_get_value_by_name(reuslt, result_len, AI_VOICE_HOST_KEY, (int *)&voiceUrl_len, &type);
    if(voiceUrl != NULL && voiceUrl_len){
       qm_iot_audio_uri_set(voiceUrl, voiceUrl_len);
    }

    agoraUrl = qm_json_get_value_by_name(reuslt, result_len, AGORA_HOST_KEY, (int *)&agoraUrl_len, &type);
    if(agoraUrl != NULL && agoraUrl_len){
       qm_iot_agora_uri_set(agoraUrl, agoraUrl_len);
    }
    
#endif

#if CONFIG_QM_IOT_MQTT_SUPPORT

    dynreg_recv.data.device_info.mqtt_host = qm_json_get_value_by_name(reuslt, result_len, SHADOW_HOST_KEY, (int *)&dynreg_recv.data.device_info.mqtt_host_len, &type);
    if(dynreg_recv.data.device_info.mqtt_host == NULL ||
        !dynreg_recv.data.device_info.mqtt_host_len){
        return -QM_EINVAL;
    }

    dynreg_recv.data.device_info.server_cert = qm_json_get_value_by_name(reuslt, result_len, SHADOW_CA_KEY, 
                                                                        (int *)&dynreg_recv.data.device_info.server_cert_len, &type);
    if(dynreg_recv.data.device_info.server_cert == NULL || 
        !dynreg_recv.data.device_info.server_cert_len){
        return -QM_EINVAL;
    }

    dynreg_recv.data.device_info.client_crt = qm_json_get_value_by_name(reuslt, result_len, SHADOW_CLIENT_CA_KEY,
                                                                        (int *)&dynreg_recv.data.device_info.client_cert_len, &type);
    if(dynreg_recv.data.device_info.client_crt == NULL || 
        !dynreg_recv.data.device_info.client_cert_len){
        return -QM_EINVAL;
    }

    dynreg_recv.data.device_info.client_private_key = qm_json_get_value_by_name(reuslt, result_len, SHADOW_CLIENT_KEY, 
                                                                                (int *)&dynreg_recv.data.device_info.client_privkey_len, &type);
    if(dynreg_recv.data.device_info.client_private_key == NULL || 
        !dynreg_recv.data.device_info.client_privkey_len){
        return -QM_EINVAL;
    }

    dynreg_recv.data.device_info.public_key = qm_json_get_value_by_name(reuslt, result_len, SHADOW_PUBLIC_KEY, 
                                                                                (int *)&dynreg_recv.data.device_info.public_key_len, &type);
    if(dynreg_recv.data.device_info.public_key == NULL || 
        !dynreg_recv.data.device_info.public_key_len){
        return -QM_EINVAL;
    }
    // 

#endif

__exit:
    if(dynreg_handle->recv_handler){
        dynreg_handle->recv_handler(dynreg_handle, &dynreg_recv, dynreg_handle->userdata);
    }
    return QM_EOK;
}


void *qm_iot_dynreg_init(void)
{
    qm_iot_dynreg_handle_t *dynreg_handle = (qm_iot_dynreg_handle_t *)qm_malloc(sizeof(qm_iot_dynreg_handle_t));
    if(dynreg_handle == NULL){
        return NULL;
    }
    memset(dynreg_handle, 0, sizeof(qm_iot_dynreg_handle_t));

    dynreg_handle->req_data.pid = qm_iot_pid_get();

    return (void *)dynreg_handle;
}

int32_t qm_iot_dynreg_setopt(void *handle, qm_iot_dynreg_option_t option, void *data)
{
    qm_iot_dynreg_req_t *req_handle = NULL;
    qm_iot_dynreg_handle_t *dynreg_handle = (qm_iot_dynreg_handle_t *)handle;
    if(dynreg_handle == NULL){
        return -QM_EINVAL;
    }

    req_handle = &dynreg_handle->req_data;

    switch (option)
    {

        case QM_IOT_DYNREGOPT_HOST:
            req_handle->host = (char *)data;
        break;

        case QM_IOT_DYNREGOPT_PORT:
            memcpy(&req_handle->port, (uint16_t *)data, sizeof(uint16_t));
        break;

        case QM_IOT_DYNREGOPT_PRODUCT_SECRET:
            req_handle->product_secret = (char *)data;
        break;

        case QM_IOT_DYNREGOPT_DEVICE_SN:
            req_handle->sn = (char *)data;
        break;

        case QM_IOT_DYNREGOPT_RECV_HANDLER:
            dynreg_handle->recv_handler = (qm_iot_dynreg_recv_handler_t)data;
        break;

        case QM_IOT_DYNREGOPT_USERDATA:
            dynreg_handle->userdata = (void *)data;
        break;

        case QM_IOT_DYNREGOPT_RECV_TIMEOUT_MS:
            memcpy(&dynreg_handle->recv_timeout, (uint32_t *)data, sizeof(uint32_t));
        break;

        case QM_IOT_DYNREGOPT_MAX_RETRY_NUM:
            memcpy(&dynreg_handle->retry_num, (uint32_t *)data, sizeof(uint32_t));
        break;

        default:

        break;
    }

    return QM_EOK;
}

#if CONFIG_QM_IOT_NETWORK_TYPE_WIFI
static int iot_calc_hmac_signature(
            qm_iot_dynreg_req_t *req_handle,
            char *hmac_sigbuf,
            const int hmac_buflen,
            int timestamp)
{
    char  hmac_source[128];
    memset(hmac_source, 0, sizeof(hmac_source));


    qm_snprintf(hmac_source,
                sizeof(hmac_source) - 1,
                "pid=%d,nonce=%d,sn=%s,timestamp=%d",
                req_handle->pid,
                req_handle->nonce,
                req_handle->sn,
                timestamp);

    QM_LOGD(LOG_TAG, "| source(%d):%s", (int)strlen(hmac_source), hmac_source);

    qm_utils_hmac_sha1(hmac_source, strlen(hmac_source),
                    hmac_sigbuf,
                    req_handle->product_secret,
                    strlen(req_handle->product_secret));

    QM_LOGD(LOG_TAG, "| signature (%d):%s", (int)strlen(hmac_sigbuf), hmac_sigbuf);

    return 0;
}
#endif

int32_t qm_iot_dynreg_start(void *handle)
{
    int ret = QM_EOK;
    uint32_t timestamp = 0;
    char signature[64 + 1] = {0};
    backoff_algorithm_ctx_t backoff_ctx = {0};
    backoff_algorithm_status_t status = BACKOFF_ALGORITHM_SUCCESS;
    uint16_t next_backoff = 0; 

    char *rsp_payload = NULL;
    char post_data[DATA_MAX_STR_LEN] = {0};
 
    httpclient_t http = {0};              /* http client */
    httpclient_data_t http_data = {0};    /* http client data */
    qm_iot_dynreg_req_t *req_handle = NULL;    
    qm_iot_dynreg_handle_t *dynreg_handle = (qm_iot_dynreg_handle_t *)handle;
    if(dynreg_handle == NULL){
        return -QM_EINVAL;
    }

    req_handle = &dynreg_handle->req_data;

    rsp_payload = (char *)qm_malloc(RSP_PAYLOAD_MAX_STR_LEN);
    if(rsp_payload == NULL){
        return -QM_ENOMEM;
    }
    memset(rsp_payload, 0, RSP_PAYLOAD_MAX_STR_LEN);

    timestamp = (uint32_t)qm_now_ms();
    qm_srandom((uint32_t)qm_now_ms());

    req_handle->nonce = qm_random_get(DEVICE_DYNREG_NONCE_MAX);

#if CONFIG_QM_IOT_NETWORK_TYPE_WIFI
    iot_calc_hmac_signature(req_handle, signature, sizeof(signature), timestamp);

    qm_snprintf(post_data, DATA_MAX_STR_LEN, SHADOW_DYNERG_REQ_FORMAT, 
        req_handle->sn, req_handle->pid, timestamp, 
        signature, req_handle->nonce);
#endif    

#if CONFIG_QM_IOT_NETWORK_TYPE_4G
#if CONFIG_QM_IOT_DYNREG_4G_V1
    qm_snprintf(post_data, DATA_MAX_STR_LEN, 
        SHADOW_DYNERG_REQ_FORMAT, req_handle->pid, qm_iot_authcode_get());

#else
    qm_snprintf(post_data, DATA_MAX_STR_LEN, 
        SHADOW_DYNERG_REQ_FORMAT, req_handle->pid, qm_iot_sn_get(), qm_iot_authcode_get());
#endif
#endif

    //链接connect
    backoff_algorithm_init(&backoff_ctx, BACKOFF_BASE_TIME_MS, dynreg_handle->recv_timeout, dynreg_handle->retry_num);
    while (1)
    {

        memset(&http, 0, sizeof(httpclient_t));
        memset(&http_data, 0, sizeof(httpclient_data_t));
        memset(rsp_payload, 0, RSP_PAYLOAD_MAX_STR_LEN);

        http_data.post_buf = post_data;
        http_data.post_buf_len = strlen(post_data);
        http_data.post_content_type = "application/json";
        http_data.response_buf = rsp_payload;
        http_data.response_buf_len = RSP_PAYLOAD_MAX_STR_LEN;

        ret = http_client_common(&http, req_handle->host, req_handle->port, NULL, HTTPCLIENT_POST, dynreg_handle->recv_timeout, &http_data);
        if(ret == QM_EOK && http_data.response_content_len && http.response_code == 200){
            break;
        }

        status = backoff_algorithm_get_next_delay(&backoff_ctx, &next_backoff);
        if(status != BACKOFF_ALGORITHM_SUCCESS){
            ret = -QM_ETIMEOUT;
            break;
        }

        qm_msleep(next_backoff);
    }
    
    http_client_close(&http);

    if(ret != QM_EOK){
        goto __exit;
    }

    qm_iot_dynreg_recv_handler(dynreg_handle, http_data.response_buf, http_data.response_content_len);

__exit:
    if(rsp_payload){
        qm_free(rsp_payload);
        rsp_payload = NULL;
    }

    return ret;
}

int32_t qm_iot_dynreg_deinit(void **handle)
{
    qm_iot_dynreg_handle_t **dynreg_handle = (qm_iot_dynreg_handle_t **)handle;
    if(handle == NULL || *dynreg_handle == NULL){
        return -QM_EINVAL;
    }

    qm_free(*dynreg_handle);

    *dynreg_handle = NULL;
    return QM_EOK;
}



