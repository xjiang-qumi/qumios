#include "qm_modem.h"
#include "qm_types.h"
#include "qm_errno.h"
#include "qm_log.h"
#include "qm_kernel.h"
#include "qm_config.h"

#include "ps_api.h"
#include "dev_api.h"
#include "debug.h"
#include "os_api.h"
#include "http.h"
#include "sockets.h"
#include "netdb.h"
#include "def.h"

#define CELL_LOCATION_HOST     "loc.yuge-info.com"
#define CELL_LOCATION_PORT     80
#define CELL_LOCATION_GET_FORMAT "POST /index.php?mcc=%03X&mnc=%02X&lac=%d&ci=%d&csq=%d&rssi=%d&imei=%s&key=ddZP97Na1E7tE1R7MrZFYG1ntXK6DYzY HTTP/1.1\r\nContent-Type: application/json\r\nUser-Agent: yuge http client\r\nAccept: */*\r\nHost: loc.yuge-info.com\r\nConnection: keep-alive\r\n\r\n"

#define LOG_TAG "modem"

typedef struct {
    qm_task_t task;
    qm_sem_t sem;
    qm_timer_t timer;
    qm_modem_config_t config;
} qm_modem_handle_t;

static qm_modem_handle_t *g_qm_modem_handle = NULL;

static qm_task_t task;

static void modem_task(void *args)
{
    BOOL nw_state_old = FALSE;
    BOOL nw_state = FALSE;
    
    while (1)
    {
        nw_state = ps_GetNetworkReady();
        
        if (nw_state != nw_state_old) {
            if (nw_state == TRUE) {
                if (g_qm_modem_handle->config.event_handler) {
                    g_qm_modem_handle->config.event_handler(QM_MODEM_EVENT_CONN, NULL, 0, g_qm_modem_handle->config.arg);
                }
                LOG_PRINTF("==========NETWORK OK==========\n");
            } else {
                if (g_qm_modem_handle->config.event_handler) {
                    g_qm_modem_handle->config.event_handler(QM_MODEM_EVENT_DISCONN, NULL, 0, g_qm_modem_handle->config.arg);
                }
                LOG_PRINTF("==========NETWORK WAITING==========\n");
            }
            nw_state_old = nw_state;
        }
        
        OSATaskSleep(MS2TICKS(1000));
    }
}

qm_modem_t qm_modem_init(const qm_modem_config_t *config)
{
    qm_err_t ret = QM_EOK;
    qm_task_t task = {0};
    qm_modem_handle_t *handle = NULL;

    handle = (qm_modem_handle_t*)qm_malloc(sizeof(qm_modem_handle_t));
    if (handle == NULL) {
        return NULL;
    }

    memset(handle, 0, sizeof(qm_modem_handle_t));
    memcpy(&handle->config, config, sizeof(qm_modem_config_t));

    qm_task_new_to_core(&task, "modem", modem_task, (void *)NULL, 4096, 2, 1);

    g_qm_modem_handle = handle;
    return (qm_modem_t)handle;
}

int qm_modem_get_imsi(qm_modem_t modem, char *imsi, int imsi_len)
{
    BOOL ret = FALSE;

    ret = dev_GetSIMImsi(imsi, (size_t)imsi_len);
    if (ret == TRUE) {
        return QM_EOK;
    } else {
        return -QM_ERROR;
    }
}

int qm_modem_get_csq(qm_modem_t modem, uint8_t *csq)
{
    UINT8 csq_val = 0;

    csq_val = ps_GetCsq();
    *csq = csq_val;
    
    return QM_EOK;
}

static int min(int a, int b)
{
    return (a >= b) ? b : a;
}

static void qm_modem_parse_lbs_result(const char *source, qm_modem_lbs_t *lbs)
{
    int i = 0;
    char *temp_ptr = NULL;
    char *lat_lng_ptr = NULL;

    memset(lbs, 0, sizeof(qm_modem_lbs_t));

    temp_ptr = strstr(source, "lat");
    if (temp_ptr) {
        for (i = 0; i < (int)strlen(temp_ptr); i++) {
            if (*temp_ptr >= '0' && *temp_ptr <= '9') {
                lat_lng_ptr = temp_ptr;
                break;
            }
            temp_ptr++;
        }
        if (lat_lng_ptr) {
            for (i = 0; i < (int)strlen(lat_lng_ptr); i++) {
                if ((*lat_lng_ptr >= '0' && *lat_lng_ptr <= '9') || *lat_lng_ptr == '.') {
                    lbs->latitude[i] = *lat_lng_ptr;
                } else {
                    break;
                }
                lat_lng_ptr++;
            }
            lbs->latitude[i] = '\0';
        }
    }

    temp_ptr = strstr(source, "lng");
    if (temp_ptr) {
        for (i = 0; i < (int)strlen(temp_ptr); i++) {
            if (*temp_ptr >= '0' && *temp_ptr <= '9') {
                lat_lng_ptr = temp_ptr;
                break;
            }
            temp_ptr++;
        }
        if (lat_lng_ptr) {
            for (i = 0; i < (int)strlen(lat_lng_ptr); i++) {
                if ((*lat_lng_ptr >= '0' && *lat_lng_ptr <= '9') || *lat_lng_ptr == '.') {
                    lbs->longitude[i] = *lat_lng_ptr;
                } else {
                    break;
                }
                lat_lng_ptr++;
            }
            lbs->longitude[i] = '\0';
        }
    }
}

static void qm_modem_lbs_task(void *args)
{
    int fd;
    int retry_count = 3;
    struct hostent *host_entry;
    struct sockaddr_in server;
    int timeout;
    char write_buf[1024] = {0};
    char read_buf[1024] = {0};
    char imei[64] = {0};
    PsLteScellInfo lte_s_info = {0};
    char *resultPtr = NULL;
    qm_modem_lbs_t lbs = {0};

    dev_GetImei(imei, sizeof(imei));
    ps_GetServerCellInfo(&lte_s_info, 2);

    host_entry = gethostbyname(CELL_LOCATION_HOST);
    if (host_entry == NULL) {
        LOG_PRINTF("%s: gethostbyname failed\n", __func__);
        qm_task_exit(NULL);
        return;
    }

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(CELL_LOCATION_PORT);
    memcpy(&server.sin_addr.s_addr, host_entry->h_addr_list[0], min(host_entry->h_length, sizeof(server.sin_addr.s_addr)));

    while (retry_count) {
        fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
            LOG_PRINTF("%s: socket failed\n", __func__);
            qm_task_exit(NULL);
            return;
        }

        timeout = 25 * 1000;
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

        if (connect(fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
            LOG_PRINTF("%s: connect failed\n", __func__);
            close(fd);
            retry_count--;
            if (retry_count) {
                OSATaskSleep(MS2TICKS(200));
            }
            continue;
        }

        sprintf(write_buf, CELL_LOCATION_GET_FORMAT,
                lte_s_info.mcc, lte_s_info.mnc, lte_s_info.tac,
                lte_s_info.cellId, ps_GetCsq(), lte_s_info.rsrp - 140, imei);

        if (send(fd, write_buf, strlen(write_buf), 0) < 0) {
            LOG_PRINTF("%s: send failed\n", __func__);
            close(fd);
            qm_task_exit(NULL);
            return;
        }

        recv(fd, read_buf, sizeof(read_buf), 0);
        close(fd);

        resultPtr = strstr(read_buf, "\"result\"");
        if (resultPtr && !strstr(resultPtr, "error")) {
            qm_modem_parse_lbs_result(read_buf, &lbs);

            if (lbs.latitude[0] != '\0' && lbs.longitude[0] != '\0') {
                QM_LOGD("1", "LBS: latitude=%s, longitude=%s\n", lbs.latitude, lbs.longitude);
                if (g_qm_modem_handle->config.event_handler) {
                    g_qm_modem_handle->config.event_handler(QM_MODEM_EVENT_LBS, &lbs, sizeof(lbs), g_qm_modem_handle->config.arg);
                }
                break;
            }
        }

        retry_count--;
        if (retry_count) {
            OSATaskSleep(MS2TICKS(200));
        }
    }

    qm_task_exit(NULL);
}

int qm_modem_get_lbs(qm_modem_t modem)
{
    qm_modem_handle_t *handle = (qm_modem_handle_t *)modem;
    qm_err_t ret;

    if (handle == NULL) {
        return -QM_ERROR;
    }

    if (ps_GetNetworkReady() != TRUE) {
        LOG_PRINTF("%s: network not ready\n", __func__);
        return -QM_ERROR;
    }

    ret = qm_task_new_to_core(&handle->task, "lbs", qm_modem_lbs_task, (void *)NULL, 4096, 2, 1);
    if (ret != QM_EOK) {
        return ret;
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
