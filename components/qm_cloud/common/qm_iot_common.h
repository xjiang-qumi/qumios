#ifndef __QM_IOT_COMMON_H__
#define __QM_IOT_COMMON_H__

#if defined(__cplusplus)
extern "C" {
#endif


#define QM_IOT_COMMON_DID_MAX_LEN       (16)
#define QM_IOT_COMMON_TOPIC_MAX_LEN     (64)
#define QM_IOT_COMMON_MSG_ID_MAX_NUM    (65535)


#define QM_IOT_COMMON_ID_KEY                "id"
#define QM_IOT_COMMON_PARAMS_KEY            "params"

#define QM_IOT_COMMON_PARAMS_SET_KEY        "set"
#define QM_IOT_COMMON_PARAMS_GET_KEY        "get"
#define QM_IOT_COMMON_PARAMS_REPORT_KEY     "report"

typedef enum {
    QM_IOT_CODE_SUCCESS,
    QM_IOT_CODE_ERROR,
}qm_iot_code_t;


#if defined(__cplusplus)
}
#endif

#endif  /* __QM_IOT_COMMON_H__ */

