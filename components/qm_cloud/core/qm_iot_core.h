#ifndef __QM_IOT_CORE_CORE_H__
#define __QM_IOT_CORE_CORE_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include "qm.h"
#include "qm_iot_api.h"
#include "qm_spec_core.h"

#ifndef CONFIG_QM_IOT_CORE_TASK_USER_CORE
#define CONFIG_QM_IOT_CORE_TASK_USER_CORE       (1)
#endif

#ifndef CONFIG_QM_IOT_CORE_TASK_PRO
#define CONFIG_QM_IOT_CORE_TASK_PRO             (10)
#endif

#ifndef CONFIG_QM_IOT_CORE_TASK_SIZE
#define CONFIG_QM_IOT_CORE_TASK_SIZE           (8144)
#endif

#ifndef CONFIG_QM_IOT_CORE_TASK_QUEUE_NUM
#define CONFIG_QM_IOT_CORE_TASK_QUEUE_NUM       (10)
#endif

#ifndef CONFIG_QM_IOT_CORE_DYNREG_PORT
#define CONFIG_QM_IOT_CORE_DYNREG_PORT         (17605)
#endif

#ifndef CONFIG_QM_IOT_CORE_DYNREG_RECVTIMEOUT
#define CONFIG_QM_IOT_CORE_DYNREG_RECVTIMEOUT  (10 * 1000)
#endif

#ifndef CONFIG_QM_IOT_CORE_DYNREG_RETRY_COUNT
#define CONFIG_QM_IOT_CORE_DYNREG_RETRY_COUNT   (5)
#endif

#ifndef CONFIG_QM_IOT_CORE_MQTT_KEEPLIVE_S
#define CONFIG_QM_IOT_CORE_MQTT_KEEPLIVE_S      (60)
#endif

#ifndef CONFIG_QM_IOT_CORE_MQTT_REPUB_MS
#define CONFIG_QM_IOT_CORE_MQTT_REPUB_MS        (1 * 1000)
#endif

#ifndef CONFIG_QM_IOT_CORE_MQTT_CID_LEN
#define CONFIG_QM_IOT_CORE_MQTT_CID_LEN        (128)
#endif

#ifndef CONFIG_QM_IOT_CORE_PROV_TIMEOUT_S
#define CONFIG_QM_IOT_CORE_PROV_TIMEOUT_S      (30 * 60)
#endif

typedef enum 
{
    QM_IOT_CORE_MSG_TYPE_WEATHER_REQUEST,
    
    QM_IOT_CORE_MSG_TYPE_AWS_REPORT,
    QM_IOT_CORE_MSG_TYPE_AWS_REPORT_FORCE,

    QM_IOT_CORE_MSG_TYPE_GET_REQ,
    QM_IOT_CORE_MSG_TYPE_GET_RSP,

    QM_IOT_CORE_MSG_TYPE_REPORT,
    QM_IOT_CORE_MSG_TYPE_REPORT_FORCE,
}qm_iot_core_msg_type_t;

void *qm_iot_core_init(void *api_handle);

int32_t qm_iot_core_stop(void *handle);

int32_t qm_iot_core_start(void *handle);

int32_t qm_iot_core_deinit(void **handle);

int32_t qm_iot_core_event_notify(void *handle, qm_iot_event_type_t event);

int32_t qm_iot_core_send(void *handle, uint32_t did, qm_iot_core_msg_type_t msg_type, qm_spec_property_operation_t *property_operation);

#if defined(__cplusplus)
}
#endif

#endif  /* __QM_IOT_CORE_CORE_H__ */