#include "qm.h"
#include "qm_iot_api.h"
#include "qm_ble_gap.h"
#include "qm_cli.h"
#include "qm_kv.h"
#include "qm_utils_string.h"

#include "qm_work.h"
#include "qm_event.h"
#include "qm_wifi.h"

#define LOG_TAG "APP"

#define KV_TAG "QM_CLOUD"

typedef struct 
{
    int pid;
    char sercet[64];
}xjiang_cloud_cfg_t;

static void *iot_handle = NULL;
static bool_t connect = QM_FALSE;
static xjiang_cloud_cfg_t g_cloud_cfg = {0};

static void handle_meminfo(char *pwbuf, int blen, int argc, char **argv);
static void handle_resetinfo(char *pwbuf, int blen, int argc, char **argv);
static void handle_sta_rssi(char *pwbuf, int blen, int argc, char **argv);
static void handle_pid_sercet(char *pwbuf, int blen, int argc, char **argv);

static struct qm_cli_command meminfo_key = {
    .name = "meminfo",
    .help = "meminfo",
    .function = handle_meminfo
};

static struct qm_cli_command resetinfo_key = {
    .name = "reset",
    .help = "cloud reset",
    .function = handle_resetinfo
};

static struct qm_cli_command sta_rssi_key = {
    .name = "sta_rssi",
    .help = "get sta rssi",
    .function = handle_sta_rssi
};

static struct qm_cli_command pid_secert_key = {
    .name = "pid",
    .help = "pid set <pid> <sercet>",
    .function = handle_pid_sercet
};


static void handle_meminfo(char *pwbuf, int blen, int argc, char **argv)
{
    qm_cli_printf("Memory Free: %lu K!!!", (qm_free_mem_get() / 1024));
}

static void handle_resetinfo(char *pwbuf, int blen, int argc, char **argv)
{
    qm_iot_reset(iot_handle);
}

static void handle_sta_rssi(char *pwbuf, int blen, int argc, char **argv)
{
    int8_t rssi = 0;

    if(!connect){
        qm_cli_printf("WARING:device not connect wifi!!!");
        return ;
    }

    qm_wifi_sta_get_rssi(&rssi);

    qm_cli_printf("wifi rssi:%d!!!", (int)rssi);
}

static void handle_pid_sercet(char *pwbuf, int blen, int argc, char **argv)
{
    if(argc == 2){
        qm_cli_printf("pid=%d.\r\n", g_cloud_cfg.pid);
        qm_cli_printf("sercet=%s.\r\n", g_cloud_cfg.sercet);
    }else if (argc == 4) {
        g_cloud_cfg.pid = atoi(argv[2]);
        memset(g_cloud_cfg.sercet, 0, 64);
        memcpy(g_cloud_cfg.sercet, argv[3], strlen(argv[3]));
        qm_kv_set(KV_TAG, &g_cloud_cfg, sizeof(xjiang_cloud_cfg_t), 1);
        qm_cli_printf("OK\r\n");
    } else {
        qm_cli_printf("Error\r\n");
        return;
    }
}

static void qm_iot_event_handler(void *handle, const qm_iot_event_t *event, void *userdata)
{
    static qm_iot_event_type_t last_type;
    if(handle == NULL || event == NULL){
        return ;
    }

    switch (event->type)
    {
        case QM_IOT_EVENT_CONFIG_TIMEOUT:
            QM_LOGD(LOG_TAG, "QM_IOT_EVENT_CONFIG_TIMEOUT");
            connect = QM_FALSE;
        break;

        case QM_IOT_EVENT_CONFIG:
            QM_LOGD(LOG_TAG, "QM_IOT_EVENT_CONFIG");
            connect = QM_FALSE;
        break;

        case QM_IOT_EVENT_CLOUD_CONN:
            QM_LOGD(LOG_TAG, "QM_IOT_EVENT_CLOUD_CONN");
            connect = QM_TRUE;
        break;

        case QM_IOT_EVENT_CLOUD_DISCONN:
            QM_LOGD(LOG_TAG, "QM_IOT_EVENT_CLOUD_DISCONN");
            connect = QM_FALSE;
        break;

        case QM_IOT_EVENT_LOCAL_CONN:
            QM_LOGD(LOG_TAG, "QM_IOT_EVENT_LOCAL_CONN");
            connect = QM_FALSE;
        break;

        case QM_IOT_EVENT_LOCAL_DISCONN:
            QM_LOGD(LOG_TAG, "QM_IOT_EVENT_LOCAL_DISCONN");
            connect = QM_FALSE;
        break;

        case QM_IOT_EVENT_RESET:
            QM_LOGD(LOG_TAG, "QM_IOT_EVENT_RESET");
            connect = QM_FALSE;
        break;

        default:
        break;
    }

}

static void qm_iot_recv_handler(void *handle, const qm_iot_recv_t *recv, void *userdata)
{
    if(handle == NULL || recv == NULL){
        return ;
    }

    QM_LOGD(LOG_TAG, "app recv type %d siid %d piid %d!!!", recv->type, recv->siid, recv->piid);

}

qm_err_t xjiang_cloud_multi_report(qm_spec_property_operation_t *operation)
{
    if(connect == QM_FALSE){
        qm_spec_property_operation_delete(operation);
        return -QM_EIO;
    }

    return qm_iot_report(iot_handle, operation);
}

qm_err_t xjiang_cloud_report(qm_spec_property_t *property)
{
    static qm_spec_property_operation_t *property_operation = NULL;
    qm_spec_property_t *property_pre = NULL;

    if(connect == QM_FALSE){
        return -QM_EIO;
    }

    if(property_operation == NULL){
        property_operation = qm_spec_property_operation_creat();
    }

    if(property_operation == NULL){
        return -QM_ENOMEM;
    }

    property_pre = qm_spec_property_creat();
    if(property_pre == NULL){
        return -QM_ENOMEM;
    }

    qm_spec_property_copy(property_pre, property);

    qm_spec_property_add(property_operation, property_pre);

    if(property->next == NULL){
        qm_iot_report(iot_handle, property_operation);
        property_operation = NULL;
    }

    return QM_EOK;
}

int xjiang_cloud_reset(void)
{
    return qm_iot_reset(iot_handle);
}

void qm_application_start(void)
{   
    int len = sizeof(xjiang_cloud_cfg_t);
    uint32_t version = 1;
  
    qm_cli_register_command(&meminfo_key);
    qm_cli_register_command(&resetinfo_key);
    qm_cli_register_command(&sta_rssi_key);
    qm_cli_register_command(&pid_secert_key);

    qm_kv_get(KV_TAG, &g_cloud_cfg, &len);

    iot_handle = qm_iot_init();
    if(iot_handle == NULL){
        QM_LOGE(LOG_TAG, "qm_api_init failed !!!");
        return ;
    }
  
    qm_iot_setopt(iot_handle, QM_IOT_OPT_PRODUCT_ID, &g_cloud_cfg.pid);
    qm_iot_setopt(iot_handle, QM_IOT_OPT_VERSION, &version);
    qm_iot_setopt(iot_handle, QM_IOT_OPT_PRODUCT_SECRET, g_cloud_cfg.sercet);
    qm_iot_setopt(iot_handle, QM_IOT_OPT_RECV_HANDLER, qm_iot_recv_handler);
    qm_iot_setopt(iot_handle, QM_IOT_OPT_EVENT_HANDLER, qm_iot_event_handler);

    qm_iot_start(iot_handle);

}

