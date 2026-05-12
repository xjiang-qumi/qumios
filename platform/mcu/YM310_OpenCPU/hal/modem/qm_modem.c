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

int qm_modem_get_lbs(qm_modem_t modem)
{
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
