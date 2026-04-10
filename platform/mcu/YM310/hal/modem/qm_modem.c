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

#define LOG_TAG "modem"

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