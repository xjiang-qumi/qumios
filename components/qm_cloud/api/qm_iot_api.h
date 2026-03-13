#ifndef __QM_IOT_CLOUD_H__
#define __QM_IOT_CLOUD_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include "qm.h"
#include "qm_spec_api.h"
#include "qm_iot_config.h"

typedef enum {
    QM_IOT_EVENT_NONE = 0,

    /**
     * @brief 正在连接云端
     */
    QM_IOT_EVENT_DEINIT = 0,

    /**
     * @brief 正在连接云端
     */
    QM_IOT_EVENT_CONNING_CLOUD,
    /**
     * @brief 云端连接成功
     */
    QM_IOT_EVENT_CLOUD_CONN,
    /**
     * @brief 云端断连
     */
    QM_IOT_EVENT_CLOUD_DISCONN,
    /**
     * @brief 正在连接路由器
     */
    QM_IOT_EVENT_CONNING_LOCAL,
    /**
     * @brief 路由器已连接
     */
    QM_IOT_EVENT_LOCAL_CONN,
    /**
     * @brief 断开路由器
     */
    QM_IOT_EVENT_LOCAL_DISCONN,

    /**
     * @brief 绑定通知
     */
    QM_IOT_EVENT_BIND_NOTIFY,

    /**
     * @brief 配网重启
     */
    QM_IOT_EVENT_RESET,
    /**
     * @brief 配网状态
     */
    QM_IOT_EVENT_CONFIG,
    /**
     * @brief 配网超时
     */
    QM_IOT_EVENT_CONFIG_TIMEOUT,
    /**
     * @brief OTA 准备
     */
    QM_IOT_EVENT_OTA_READY,
    /**
     * @brief OTA 开始
     */
    QM_IOT_EVENT_OTA_START,
    /**
     * @brief OTA 下载中
     */
    QM_IOT_EVENT_OTA_INSTALLING,

    /**
     * @brief OTA 进度更新
     */
    QM_IOT_EVENT_OTA_PROGRESS_UPADTE,

    /**
     * @brief OTA 失败
     */
    QM_IOT_EVENT_OTA_FAILED,
    /**
     * @brief OTA 完成
     */
    QM_IOT_EVENT_OTA_SUCCESS,

    /**
     * @brief OTA 取消
     */
    QM_IOT_EVENT_OTA_CANCEL,

    /**
     * @brief 天气通知
     * 
     * @details 
     * 
     */
    QM_IOT_EVENT_WEATHER_NOTIFY,

    QM_IOT_EVENT_MAX
} qm_iot_event_type_t;

typedef enum {
    /**
     * @brief 接收类型为 AWS IOT  (后续会逐步取消此接口)
 * 
     * 
     * @details 
     * 
     */
    QM_IOT_RECV_TYPE_SET = 0,

#if CONFIG_QM_IOT_SPEC_SUPPORT
    /**
     * @brief 接收类型为 QM IOT 下发
     * 
     * @details 
     * 
     */
    QM_IOT_RECV_TYPE_PROPERTY_SET,


    /**
     * @brief 接收类型为 平台回复设备的get请求信息
     * 
     * @details 
     * 
     */
    QM_IOT_RECV_TYPE_PROPERTY_GET_RSP,

    /**
     * @brief 接收类型为 平台向设备请求信息
     * 
     * @details 
     * 
     */
    QM_IOT_RECV_TYPE_PROPERTY_GET_REQ,

#endif

    QM_IOT_RECV_TYPE_MAX
} qm_iot_recv_type_t;

typedef struct {
    /**
     * @brief 接收类型
     * 
     * @details 
     * 
     */
    qm_iot_recv_type_t type;
    /**
     * @brief 设备DID
     * 
     * @details 
     * 
     */
    uint32_t did;
    /**
     * @brief SIID
     * 
     * @details 
     * 
     */
    uint8_t siid;
    /**
     * @brief PIID
     * 
     * @details 
     * 
     */
    uint16_t piid;
    union {
        /**
         * @brief 小匠spec格式
         * 
         * @details 
         * 
         */
        struct {
            qm_spec_property_t *property;
        }set;

        //平台回复设备的get请求信息
        struct {
            qm_spec_property_t *property;
        }get_rsp;

        //平台向设备请求信息
        struct {
            qm_spec_property_t *property;
        }get_req;

    } data;
} qm_iot_recv_t;

typedef struct {
    /**
     * @brief 事件类型
     * 
     * @details 
     * 
     */
    qm_iot_event_type_t type;
    union {
        /**
         * @brief QM_IOT_EVENT_OTA_READY
         * 
         * @note OTA开始前,会触发QM_IOT_EVENT_OTA_READY事件
         *       若开发者无需进行OTA升级则对其进行置位
         */
        struct 
        {
            uint8_t is_busy;
        }ota_ready;

        /**
         * @brief QM_IOT_EVENT_OTA_PROGRESS_UPADTE
         * 
         * @note 升级进度 0~100%
         */
        struct 
        {
            uint16_t progress_details;
        }ota_progress_update;

        /**
         * @brief QM_IOT_EVENT_WEATHER_NOTIFY
         * 
         * @details 
         * 
         */
        struct 
        {
            int weather_type;     //天气类型
            int temp;                               //温度
            int humidity;                           //湿度
            int city_id;                            //城市ID

            char *condition;                        //天气状态
            int condition_len;
        
            char *country_name;                     //国家
            int country_name_len;                     //国家
        
            char *pname;                            //省份
            int pname_len;
        
            int secondaryname_len;
            char *secondaryname;                    //次级名称
        
            int name_len;
            char *name;                             //具体区域
        
            int sunset_len;
            char *sunset;                           //日落时间
        
            int sunrise_len;
            char *sunrise;                          //日升时间
            
        }weather_notify;
        
    } data;
} qm_iot_event_t;


/**
 * @brief 通知用户所调用的数据接收回调函数
 * 
 * @param[in] handle 会话句柄
 * @param[in] recv 消息结构体, 存放收到的dev报文内容
 * @param[in] userdata 用户上下文
 * 
 * @return void
 */
typedef void (*qm_iot_recv_handler_t)(void *handle, const qm_iot_recv_t *recv, void *userdata);

/**
 * @brief 通知用户所调用的相关事件
 * 
 * @details 
 * 
 */
typedef void (*qm_iot_event_handler_t)(void *handle, const qm_iot_event_t *event, void *userdata);


typedef enum {

    /**
     * @brief 设备的Version, 可从物联网平台控制台</a>获取
     *
     * @details
     *
     * 数据类型: (char *)
     */
    QM_IOT_OPT_VERSION,

    /**
     * @brief 设备的product id, 可从物联网平台控制台</a>获取
     *
     * @details
     *
     * 数据类型: (uint32_t *)
     */
    QM_IOT_OPT_PRODUCT_ID,

    /**
     * @brief 设备的productSecret, 可从物联网平台控制台</a>获取
     *
     * @details
     *
     * 数据类型: (char *)
     */
    QM_IOT_OPT_PRODUCT_SECRET,


    /**
     * @brief 用户需要SDK暂存的上下文
     *
     * @details
     *
     * 这个上下文指针会在 QM_IOT_OPT_RECV_HANDLER 和 QM_IOT_OPT_EVENT_HANDLER 设置的回调被调用时, 由SDK传给用户
     *
     * 数据类型: (void *)
     */
    QM_IOT_OPT_USERDATA,

    /**
     * @brief 从CLOUD服务器收取的数据从此默认回调函数进行通知
     *
     * 
     * 数据类型: ( @ref qm_iot_recv_handler_t )
     */
    QM_IOT_OPT_RECV_HANDLER,

    /**
     * @brief 事件会从此回调函数进行通知, 如上线/断线/重新上线/配网状态等信息
     *
     * @details
     *
     * 数据类型: ( @ref qm_iot_event_handler_t )
     */
    QM_IOT_OPT_EVENT_HANDLER,

    QM_IOT_OPT_MAX
} qm_iot_option_t;

/**
 * @brief 初始化IOT句柄
 * 
 * @details 
 * 
 */
void *qm_iot_init(void);

/**
 * @brief 反初始化IOT句柄
 * 
 * @param[in] handle IOT会话句柄
 * 
 * @details 
 * 
 */
int32_t qm_iot_deinit(void **handle);

/**
 * @brief 配合会话
 * 
 * @details 
 * 
 */
int32_t qm_iot_setopt(void *handle, qm_iot_option_t option, void *data);

/**
 * @brief 启动IOT模块
 * 
 * @details 
 * 
 */
int32_t qm_iot_start(void *handle);

/**
 * @brief 停止IOT模块
 * 
 * @details 
 * 
 */
int32_t qm_iot_stop(void *handle);


/**
 * @brief 上报 AWS spec属性 (后续会逐步取消此接口)
 * 
 * @details property_operation调用后，接口内部会qm_free掉该资源
 * 
 */
int32_t qm_iot_report(void *handle, qm_spec_property_operation_t *property_operation);

#if CONFIG_QM_IOT_SPEC_SUPPORT
/**
 * @brief 上报 QM 标准 协议 spec属性
 * 
 * @details property_operation调用后，接口内部会qm_free掉该资源
 * 
 */
int32_t qm_iot_property_report(void *handle, qm_spec_property_operation_t *property_operation);


/**
 * @brief GET QM 标准 协议 spec属性, 设备向平台请求信息
 * 
 * @details property_operation调用后，接口内部会qm_free掉该资源
 * 
 */
int32_t qm_iot_property_get_request(void *handle, qm_spec_property_operation_t *property_operation);


/**
 * @brief GET QM 标准 协议 spec属性, 设备回复平台的get请求信息
 * 
 * @details property_operation调用后，接口内部会qm_free掉该资源
 * 
 */
int32_t qm_iot_property_get_rsponse(void *handle, qm_spec_property_operation_t *property_operation);
#endif
/**
 * @brief 重置IOT模块
 * 
 * @details 内部自带重启指令
 * 
 */
int32_t qm_iot_reset(void *handle);

/**
 * @brief IOT模块向天气进行请求
 * 
 * @details 
 * 
 */
int32_t qm_iot_set_weahter_request(void *handle);

/**
 * @brief 上报spec属性
 * 
 * @details property_operation调用后，接口内部会qm_free掉该资源
 * 
 */
int32_t qm_iot_event_notify(void *handle, qm_iot_event_type_t event);

#if defined(__cplusplus)
}
#endif

#endif  /* __QM_IOT_CLOUD_H__ */