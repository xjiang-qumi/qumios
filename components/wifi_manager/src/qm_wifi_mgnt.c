#include "qm_wifi_mgnt.h"
#include "qm_wifi_dev.h"
#include "qm_errno.h"
#include "qm_kv.h"
#include "qm_event.h"
#include "qm_work.h"

typedef enum {
    QM_WIFI_STATE_NONE,
    QM_WIFI_STATE_CONNECTING,
    QM_WIFI_STATE_CONNECTED,
    QM_WIFI_STATE_DISCONNECTED,
}qm_wifi_state_t;
typedef struct {

    qm_wifi_dev_t *wifi_dev;
    qm_wifi_state_t wifi_state;
}wifi_mgnt_ctx_t;

static wifi_mgnt_ctx_t g_wifi_mgnt_ctx = {0};


static void wifi_event_hander(qm_wifi_event_t event, qm_wifi_event_info_t *event_info, void *arg)
{
    switch(event){

        case QM_WIFI_EVENT_READY:

            qm_event_post(QM_EVENT_WIFI, (uint16_t)event, 0, 0);
        break;

        case QM_WIFI_EVENT_STA_START:
        case QM_WIFI_EVENT_STA_STOP:
        
            qm_event_post(QM_EVENT_WIFI, (uint16_t)event, 0, 0);
        break;

        case QM_WIFI_EVENT_STA_CONNECTED:
            qm_event_post(QM_EVENT_WIFI, (uint16_t)event, 0, 0);

        break;

        case QM_WIFI_EVENT_STA_GOT_IP:

            qm_event_post(QM_EVENT_WIFI, (uint16_t)event, event_info, sizeof(qm_wifi_event_info_t));
        break;

        case QM_WIFI_EVENT_STA_LOST_IP:

            qm_event_post(QM_EVENT_WIFI, (uint16_t)event, 0, 0);
        break;

        case QM_WIFI_EVENT_STA_DISCONNECTED:
            g_wifi_mgnt_ctx.wifi_state = QM_WIFI_STATE_DISCONNECTED;
            qm_event_post(QM_EVENT_WIFI, (uint16_t)event, event_info, sizeof(qm_wifi_event_info_t));

        break;

        case QM_WIFI_EVENT_AP_START:
        case QM_WIFI_EVENT_AP_STOP:
            qm_event_post(QM_EVENT_WIFI, (uint16_t)event, 0, 0);
        break;

        case QM_WIFI_EVENT_AP_STACONNECTED:
        case QM_WIFI_EVENT_AP_STAIPASSIGNED:
        case QM_WIFI_EVENT_AP_STADISCONNECTED:

            qm_event_post(QM_EVENT_WIFI, (uint16_t)event, event_info, sizeof(qm_wifi_event_info_t));
        break;  

        case QM_WIFI_EVENT_SCAN_DONE:
        
            qm_event_post(QM_EVENT_WIFI, (uint16_t)event, event_info, sizeof(qm_wifi_event_info_t));
        break;        

        default:
            break;
    }
}

int qm_wifi_init(void)
{
    static int init = 0;
    qm_err_t ret = QM_EOK;

    if(init){
        return QM_EOK;
    }

    g_wifi_mgnt_ctx.wifi_dev = qm_wifi_dev_get();
    if(g_wifi_mgnt_ctx.wifi_dev == NULL){
        return -QM_EINVAL;
    }

    ret = qm_wifi_dev_register_event_handler(g_wifi_mgnt_ctx.wifi_dev, wifi_event_hander, NULL);
    if(ret != QM_EOK){
        return ret;
    }

    ret = qm_wifi_dev_init(g_wifi_mgnt_ctx.wifi_dev);
    if(ret != QM_EOK){
        return ret;
    }

    init = 1;
    return ret;
}

int qm_wifi_deinit(void)
{
    return qm_wifi_dev_deinit(g_wifi_mgnt_ctx.wifi_dev);
}

int qm_wifi_set_mode(qm_wifi_mode_t mode)
{
    return qm_wifi_dev_set_mode(g_wifi_mgnt_ctx.wifi_dev, mode);
}

int qm_wifi_get_mode(qm_wifi_mode_t *mode)
{
    return qm_wifi_dev_get_mode(g_wifi_mgnt_ctx.wifi_dev, mode);
}

int qm_wifi_set_config(qm_wifi_interface_t ifx, qm_wifi_config_t *config)
{
    return qm_wifi_dev_set_config(g_wifi_mgnt_ctx.wifi_dev, ifx, config);
}

int qm_wifi_get_config(qm_wifi_interface_t ifx, qm_wifi_config_t *config)
{
    return qm_wifi_dev_get_config(g_wifi_mgnt_ctx.wifi_dev, ifx, config);
}

int qm_wifi_start(void)
{
    return qm_wifi_dev_start(g_wifi_mgnt_ctx.wifi_dev);
}

int qm_wifi_stop(void)
{
    return qm_wifi_dev_stop(g_wifi_mgnt_ctx.wifi_dev);
}

int qm_wifi_connect(void)
{
    return qm_wifi_dev_connect(g_wifi_mgnt_ctx.wifi_dev);
}

int qm_wifi_disconnect(void)
{
    return qm_wifi_dev_disconnect(g_wifi_mgnt_ctx.wifi_dev);
}

int qm_wifi_deauth(const uint8_t mac[6])
{
    return qm_wifi_dev_deauth(g_wifi_mgnt_ctx.wifi_dev, mac);
}

int qm_wifi_scan_start(qm_scan_config_t *config)
{
    return qm_wifi_dev_scan_start(g_wifi_mgnt_ctx.wifi_dev, config);
}

int qm_wifi_scan_get_ap_records(uint16_t *number, qm_wifi_ap_record_t *ap_records)
{
    return QM_EOK;
}

int qm_wifi_scan_stop(void)
{
    return qm_wifi_dev_scan_stop(g_wifi_mgnt_ctx.wifi_dev);
}

int qm_wifi_set_mac(qm_wifi_interface_t ifx, const uint8_t mac[6])
{
    return qm_wifi_dev_set_mac(g_wifi_mgnt_ctx.wifi_dev, ifx, mac);
}

int qm_wifi_get_mac(qm_wifi_interface_t ifx, uint8_t mac[6])
{
    return qm_wifi_dev_get_mac(g_wifi_mgnt_ctx.wifi_dev, ifx, mac);
}

int qm_wifi_sta_get_rssi(int8_t *rssi)
{
    return qm_wifi_dev_sta_get_rssi(g_wifi_mgnt_ctx.wifi_dev, rssi);
}

int qm_wifi_sta_get_ap_info(qm_wifi_ap_record_t *ap_info)
{
    return qm_wifi_dev_sta_get_ap_info(g_wifi_mgnt_ctx.wifi_dev, ap_info);
}

