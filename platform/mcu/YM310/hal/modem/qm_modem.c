#include "qm_modem.h"
#include "qm_types.h"
#include "qm_errno.h"
#include "qm_log.h"
#include "qm_kernel.h"
#include "qm_config.h"

#include "cmips.h"
#include "cmisim.h"
#include "cmimm.h"
#include "cmisms.h"
#include "yopen_sim.h"
#include "yopen_nw.h"
#include "yopen_debug.h"
#include "yopen_dev.h"
#include "HTTPClient.h"
#include "cJSON.h"
#include "qm_sprintf.h"

#define LOG_TAG "modem"
#define HTTP_LBS_URL "http://loc.yuge-info.com/index.php?mcc=%d&mnc=%d&lac=%d&ci=%d&csq=%d&rssi=%d&imei=%s&key=9GazkFj1yWG4M9MKJ1JrRKkideQDS6Z4"
#define HTTP_RSP_HEAD_BUFFER_SIZE 800
#define HTTP_RSP_CONTENT_BUFFER_SIZE (8*1024)

typedef struct {
    qm_task_t task;
    qm_sem_t sem;
    qm_timer_t timer;
    qm_modem_config_t config;
}qm_modem_handle_t;

static qm_modem_handle_t *g_qm_modem_handle = NULL;


static qm_task_t task;

static void yopen_modem_task(void *args)
{
    uint8_t nSim = 0;
    yopen_nw_reg_state_e nw_state_old = 0;
    yopen_nw_reg_status_info_s nw_status;
    while (1)
    {
        yopen_nw_get_reg_status(nSim, &nw_status);
        if (nw_status.data_reg.state != nw_state_old) {
            /* 状态变化 */
            if (nw_status.data_reg.state == YOPEN_NW_REG_STATE_HOME_NETWORK || nw_status.data_reg.state == YOPEN_NW_REG_STATE_ROAMING){
                /* 网络连接成功 */
                if(g_qm_modem_handle->config.event_handler){
                    g_qm_modem_handle->config.event_handler(QM_MODEM_EVENT_CONN, NULL, 0, g_qm_modem_handle->config.arg);
                }
                yopen_trace("==========YOPEN NETOWRK OK==========");       
            } else {
                /* 网络连接失败 */
                if(g_qm_modem_handle->config.event_handler){
                    g_qm_modem_handle->config.event_handler(QM_MODEM_EVENT_DISCONN, NULL, 0, g_qm_modem_handle->config.arg);
                }
                yopen_trace("==========YOPEN NETOWRK WAITING=========="); 
            }
            nw_state_old = nw_status.data_reg.state;
        }
        yopen_rtos_task_sleep_ms(1000);
    }
}

qm_modem_t qm_modem_init(const qm_modem_config_t *config)
{
    qm_err_t ret = QM_EOK;
    qm_task_t task = {0};
    qm_modem_handle_t *handle = NULL;

    handle = (qm_modem_handle_t*)qm_malloc(sizeof(qm_modem_handle_t));
    if(handle == NULL){
        return NULL;
    }

    memset(handle, 0, sizeof(qm_modem_handle_t));
    memcpy(&handle->config, config, sizeof(qm_modem_config_t));


    qm_task_new_to_core(&task, "yopen_modem", yopen_modem_task, (void *)NULL, 4096, 2, 1);


    g_qm_modem_handle = handle;
    return (qm_modem_t)handle;
}

int qm_modem_get_imsi(qm_modem_t modem, char *imsi, int imsi_len)
{
    int ret = 0;

    ret = yopen_sim_get_imsi(0, imsi, imsi_len);
    if (ret == 0) {
        return QM_EOK;
    } else {
        return -QM_ERROR;
    }
}

int qm_modem_get_csq(qm_modem_t modem, uint8_t *csq)
{
    int ret = 0;

    ret = yopen_nw_get_csq(0, &csq);
    if (ret == 0) {
        return QM_EOK;
    } else {
        return -QM_ERROR;
    }
}

static HttpClientData* _http_malloc_data(void)
{
    HttpClientData* clientData = (HttpClientData*)qm_malloc(sizeof(HttpClientData));
    if (clientData == NULL) {
        return NULL;
    }
    memset(clientData, 0, sizeof(HttpClientData));
    clientData->headerBufLen = HTTP_RSP_HEAD_BUFFER_SIZE;
    clientData->headerBuf = (char*)qm_malloc(HTTP_RSP_HEAD_BUFFER_SIZE);
    if (clientData->headerBuf == NULL) {
        qm_free(clientData);
        return NULL;
    }
    clientData->respBufLen = HTTP_RSP_CONTENT_BUFFER_SIZE;
    clientData->respBuf = (char*)qm_malloc(HTTP_RSP_CONTENT_BUFFER_SIZE);
    if (clientData->respBuf == NULL) {
        qm_free(clientData->headerBuf);
        qm_free(clientData);
        return NULL;
    }
    return clientData;
}

static void _http_free_data(HttpClientData* clientData)
{
    if (!clientData) {
        return;
    }
    if (clientData->headerBuf) {
        qm_free(clientData->headerBuf);
    }
    if (clientData->respBuf) {
        qm_free(clientData->respBuf);
    }
    if (clientData->postBuf) {
        qm_free(clientData->postBuf);
    }
    qm_free(clientData);
}

static void parse_lbs_response(const char *json_data, qm_modem_lbs_t *modem_lbs)
{
    if (!json_data || !modem_lbs) {
        return;
    }

    cJSON *root = cJSON_Parse(json_data);
    if (!root) {
        return;
    }

    cJSON *result = cJSON_GetObjectItem(root, "result");
    if (result != NULL && cJSON_IsObject(result)) {
        cJSON *lng = cJSON_GetObjectItem(result, "lng");
        if (lng != NULL && cJSON_IsNumber(lng)) {
            snprintf_(modem_lbs->longitude, sizeof(modem_lbs->longitude), "%.6f", lng->valuedouble);
            QM_LOGD(LOG_TAG, "LBS longitude: %s", modem_lbs->longitude);
        }
        
        cJSON *lat = cJSON_GetObjectItem(result, "lat");
        if (lat != NULL && cJSON_IsNumber(lat)) {
            snprintf_(modem_lbs->latitude, sizeof(modem_lbs->latitude), "%.6f", lat->valuedouble);
            QM_LOGD(LOG_TAG, "LBS latitude: %s", modem_lbs->latitude);
        }
    }

    cJSON_Delete(root);
}

static void yopen_lbs_task(void *args)
{
    qm_modem_handle_t *handle = (qm_modem_handle_t*)args;
    yopen_nw_cell_info_s cell_info = {0};
    yopen_nw_phy_status_info_s phy_status = {0};
    uint8_t csq;
    char imei[64] = {0};
    char lbs_url[1024] = {0};
    qm_modem_lbs_t modem_lbs = {0};
    HTTPResult result;

    yopen_nw_get_cell_info(0, &cell_info);
    yopen_nw_get_csq(0, &csq);
    yopen_nw_get_phy_status_info(0, &phy_status);
    yopen_dev_get_imei(imei, 64, 0);

    if (!cell_info.lte_info_valid) {
        qm_task_exit(NULL);
        return;
    }

    snprintf(lbs_url, sizeof(lbs_url), HTTP_LBS_URL,
             cell_info.lte_info[0].mcc, cell_info.lte_info[0].mnc,
             cell_info.lte_info[0].tac, cell_info.lte_info[0].cid,
             csq, phy_status.rssi, imei);

    HttpClientContext* clientContext = (HttpClientContext*)qm_malloc(sizeof(HttpClientContext));
    if (!clientContext) {
        qm_task_exit(NULL);
        return;
    }
    memset(clientContext, 0, sizeof(HttpClientContext));
    clientContext->timeout_s = 2;
    clientContext->timeout_r = 20;
    clientContext->socket = -1;
    clientContext->pdpId = 1;
    clientContext->saveMem = 1;
    clientContext->ciphersuite[0] = 0xFFFF;

    HttpClientData* clientData = _http_malloc_data();
    if (!clientData) {
        qm_free(clientContext);
        qm_task_exit(NULL);
        return;
    }

    result = httpConnect(clientContext, lbs_url);
    if (result != HTTP_OK) {
        _http_free_data(clientData);
        qm_free(clientContext);
        qm_task_exit(NULL);
        return;
    }

    result = httpSendRequest(clientContext, lbs_url, HTTP_GET, clientData);
    if (result != HTTP_OK) {
        httpClose(clientContext);
        _http_free_data(clientData);
        qm_free(clientContext);
        qm_task_exit(NULL);
        return;
    }

    do {
        memset(clientData->headerBuf, 0, clientData->headerBufLen);
        memset(clientData->respBuf, 0, clientData->respBufLen);
        result = httpRecvResponse(clientContext, clientData);
    } while (result == HTTP_MOREDATA || result == HTTP_CONN);

    QM_LOGD(LOG_TAG, "LBS respBuf: %s", clientData->respBuf);
    parse_lbs_response(clientData->respBuf, &modem_lbs);

    if (strlen(modem_lbs.longitude) > 0 && strlen(modem_lbs.latitude) > 0) {
        if (g_qm_modem_handle->config.event_handler) {
            g_qm_modem_handle->config.event_handler(QM_MODEM_EVENT_LBS, &modem_lbs, 
                                                    sizeof(qm_modem_lbs_t), 
                                                    g_qm_modem_handle->config.arg);
        }
    }

    httpClose(clientContext);
    _http_free_data(clientData);
    qm_free(clientContext);
    qm_task_exit(NULL);
}

int qm_modem_get_lbs(qm_modem_t modem)
{
    qm_modem_handle_t *handle = (qm_modem_handle_t*)modem;
    qm_err_t ret;
    uint8_t nSim = 0;
    yopen_nw_reg_status_info_s nw_status;

    yopen_nw_get_reg_status(nSim, &nw_status);
    if (nw_status.data_reg.state == YOPEN_NW_REG_STATE_HOME_NETWORK || nw_status.data_reg.state == YOPEN_NW_REG_STATE_ROAMING) {
        ret = qm_task_new(&handle->task, "lbs", yopen_lbs_task, handle, 4096, CONFIG_QM_APP_TASK_PRIO);
        if (ret != QM_EOK) {
            return ret;
        }
    } else {
        return -QM_ERROR;
    }
    
    return QM_EOK;
}

int qm_modem_start_ppp(qm_modem_t modem, uint32_t timeout)
{
    return QM_EOK;
}

int qm_modem_restart_ppp(qm_modem_t modem, uint32_t timeout)
{
    return QM_EOK;
}

int qm_modem_stop_ppp(qm_modem_t modem, uint32_t timeout)
{
    return QM_EOK;
}