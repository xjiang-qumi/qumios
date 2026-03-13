#include "qm.h"
#include "qm_wifi.h"
#include "qm_work.h"
#include "qm_event.h"
#include "qm_errno.h"
#include "qm_log.h"

#define LOG_TAG  "wifi scan"


static void qm_event_handler(qm_input_event_t *input_event, void *arg)
{
    qm_wifi_event_info_t *event_info = (qm_wifi_event_info_t*)input_event->value;

    switch(input_event->sub_event){

        case QM_WIFI_EVENT_SCAN_DONE:

            QM_LOGD(LOG_TAG, "wifi scan done, status: %d, ap num: %d", event_info->scan_done.status, event_info->scan_done.ap_num);

            

        break;

        default:
        break;
    }
}

void qm_application_start(void)
{
    qm_event_register(QM_EVENT_WIFI, qm_event_handler, NULL);

    qm_wifi_init();

    qm_wifi_set_mode(QM_WIFI_MODE_STA);

    qm_wifi_start();

    qm_wifi_scan_start(NULL);
    
}