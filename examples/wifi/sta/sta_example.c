#include "qm.h"
#include "qm_wifi.h"
#include "qm_work.h"
#include "qm_event.h"
#include "qm_errno.h"
#include "qm_log.h"

#define LOG_TAG  "wifi sta"

#define EXAMPLE_WIFI_SSID       "xjiang"
#define EXAMPLE_WIFI_PASSWORD   "xjwl2015"

#define EXAMPLE_WIFI_RETRY_MAX_COUNT   10
static int retry_count = 0;

static void qm_event_handler(qm_input_event_t *input_event, void *arg)
{
    qm_wifi_event_info_t *event_info = (qm_wifi_event_info_t*)input_event->value;

    switch(input_event->sub_event){

        case QM_WIFI_EVENT_STA_START:
            QM_LOGD(LOG_TAG, "connect to the AP");
            qm_wifi_connect();
        break;

        case QM_WIFI_EVENT_STA_STOP:

        break;

        case QM_WIFI_EVENT_STA_CONNECTED:
            QM_LOGD(LOG_TAG, "connected to the AP");
        break;

        case QM_WIFI_EVENT_STA_GOT_IP:
            QM_LOGD(LOG_TAG, "sta got ip");

            QM_LOGD(LOG_TAG, "ip:"      QM_IPSTR, QM_IP2STR(&event_info->got_ip.ip_info.ip));
            QM_LOGD(LOG_TAG, "netmask:" QM_IPSTR, QM_IP2STR(&event_info->got_ip.ip_info.netmask));
            QM_LOGD(LOG_TAG, "gw:"      QM_IPSTR, QM_IP2STR(&event_info->got_ip.ip_info.gw));

        break;

        case QM_WIFI_EVENT_STA_LOST_IP:

        break;

        case QM_WIFI_EVENT_STA_DISCONNECTED:

            QM_LOGD(LOG_TAG, "sta disconnected reason: %d", event_info->sta_disconnected.reason);

            if (retry_count < EXAMPLE_WIFI_RETRY_MAX_COUNT) {
                qm_wifi_connect();
                retry_count++;
                QM_LOGD(LOG_TAG, "retry to connect to the AP");
            }

        break;

        default:
        break;
    }
}

void qm_application_start(void)
{

    qm_wifi_config_t config = {
        .sta = {
            .ssid = EXAMPLE_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_WIFI_SSID),
            .password = EXAMPLE_WIFI_PASSWORD,
            .password_len = strlen(EXAMPLE_WIFI_PASSWORD),
        },
    };

    qm_event_register(QM_EVENT_WIFI, qm_event_handler, NULL);

    qm_wifi_init();

    qm_wifi_set_mode(QM_WIFI_MODE_STA);
    qm_wifi_set_config(QM_WIFI_IF_STA, &config);
    qm_wifi_start();
}