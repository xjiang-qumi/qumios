#ifndef __QM_IOT_DYNREG_H__
#define __QM_IOT_DYNREG_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include "qm.h"

/**
 * @brief dynreg模块收到从网络上来的报文时, 通知用户的报文类型
 */
typedef enum {
    /**
     * @brief dynreg模块返回的http status code
     */
    QM_IOT_DYNREGRECV_STATUS_CODE,
    /**
     * @brief dynreg模块返回的设备信息
     */
    QM_IOT_DYNREGRECV_DEVICE_INFO,
} qm_iot_dynreg_recv_type_t;

/**
 * @brief dynreg模块收到从网络上来的报文时, 通知用户的报文内容
 */
typedef struct {
    /**
     * @brief 报文内容所对应的报文类型, 更多信息请参考@ref qm_iot_dynreg_recv_type_t
     */
    qm_iot_dynreg_recv_type_t  type;
    union {
        /**
         * @brief dynreg模块返回的http status code
         */
        struct {
            uint32_t code;
        } status_code;
        /**
         * @brief dynreg模块返回的设备信息
         */
        struct {

            uint32_t did;

            char *public_key;
            uint32_t public_key_len;
            char *server_cert;              /*server root certificate*/
            uint32_t server_cert_len;
            char *client_crt;               /*Client certificate*/
            uint32_t client_cert_len;
            char *client_private_key;       /*Client certificate's private key*/
            uint32_t client_privkey_len;
            char *mqtt_host;
            uint32_t mqtt_host_len;
            uint16_t mqtt_port;
        } device_info;
    } data;
} qm_iot_dynreg_recv_t;

/**
 * @brief dynreg模块收到从网络上来的报文时, 通知用户所调用的数据回调函数
 *
 * @param[in] handle dynreg会话句柄
 * @param[in] packet dynreg消息结构体, 存放收到的dynreg报文内容
 * @param[in] userdata 用户上下文
 *
 * @return void
 */
typedef void (* qm_iot_dynreg_recv_handler_t)(void *handle, const qm_iot_dynreg_recv_t *packet, void *userdata);

/**
 * @brief @ref qm_iot_dynreg_setopt 接口的option参数可选值.
 *
 * @details 下文每个选项中的数据类型, 指的是@ref qm_iot_dynreg_setopt 中, data参数的数据类型
 *
 * 1. data的数据类型是char *时, 以配置@ref QM_IOT_DYNREGOPT_HOST 为例:
 *
 *    char *host = "xxx";
 *    qm_iot_dynreg_setopt(dynreg_handle, QM_IOT_DYNREGOPT_HOST, host);
 *
 * 2. data的数据类型是其他数据类型时, 以配置@ref QM_IOT_DYNREGOPT_PORT 为例:
 *
 *    uint16_t port = 443;
 *    qm_iot_mqtt_setopt(dynreg_handle, QM_IOT_DYNREGOPT_PORT, (void *)&port);
 */
typedef enum {
    /**
     * @brief http动态注册 服务器建联时, 网络使用的安全凭据
     *
     * @details
     *
     * 该配置项用于为底层网络配置@ref qm_iot_sysdep_network_cred_t 安全凭据数据
     *
     * 应当把 @ref qm_iot_sysdep_network_cred_t 中option配置为@ref QM_IOT_SYSDEP_NETWORK_CRED_SVRCERT_CA , 以tls方式建联
     *
     * 数据类型: (qm_iot_sysdep_network_cred_t *)
     */
    QM_IOT_DYNREGOPT_NETWORK_CRED,

    /**
     * @brief http动态注册 服务器的域名地址或者ip地址
     *
     * @details
     *
     * 物联网平台 http动态注册 服务器域名地址列表:
     *
     * | 域名地址                                        | 区域    | 端口号
     * |-------------------------------------------------|---------|---------
     * | XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX              | 国内    | 443
     * | XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX              | 海外    | 443
     *
     * 数据类型: (char *)
     */
    QM_IOT_DYNREGOPT_HOST,

    /**
     * @brief http动态注册 服务器的端口号
     *
     * @details
     *
     * 连接阿里云物联网平台 http动态注册 服务器时:
     *
     * 必须使用tls方式建联, 端口号设置为443
     *
     * 数据类型: (uint16_t *)
     */
    QM_IOT_DYNREGOPT_PORT,

    /**
     * @brief 设备的productId, 可从物联网平台控制台</a>获取
     *
     * @details
     *
     * 数据类型: (uint32_t *)
     */
    QM_IOT_DYNREGOPT_PRODUCT_ID,

    /**
     * @brief 设备的productSecret, 可从物联网平台控制台</a>获取
     *
     * @details
     *
     * 数据类型: (char *)
     */
    QM_IOT_DYNREGOPT_PRODUCT_SECRET,

    /**
     * @brief 设备的deviceSn, wifi设备mac地址代替
     *
     * @details
     *
     * 数据类型: (char *)
     */
    QM_IOT_DYNREGOPT_DEVICE_SN,

    /**
     * @brief dynreg会话接收消息时可消费的最长时间间隔
     *
     * @details
     *
     * 数据类型: (uint32_t) 默认值: (5 * 1000) ms
     */
    QM_IOT_DYNREGOPT_RECV_TIMEOUT_MS,

    /**
     * @brief 设置回调, 它在SDK收到网络报文的时候被调用, 告知用户
     *
     * @details
     *
     * 数据类型: (qm_iot_dynreg_http_recv_handler_t)
     */
    QM_IOT_DYNREGOPT_RECV_HANDLER,

    /**
     * @brief 用户需要SDK暂存的上下文
     *
     * @details
     *
     * 这个上下文指针会在 QM_IOT_DYNREGOPT_RECV_HANDLER 和 QM_IOT_DYNREGOPT_EVENT_HANDLER 设置的回调被调用时, 由SDK传给用户
     *
     * 数据类型: (void *)
     */
    QM_IOT_DYNREGOPT_USERDATA,


    /**
     * @brief dynreg 模块接收重试次数
     *
     * @details
     *
     * 数据类型: (uint32_t) 默认值: 3
     */
    QM_IOT_DYNREGOPT_MAX_RETRY_NUM,

    QM_IOT_IOTDYNREGOPT_MAX
} qm_iot_dynreg_option_t;


/**
 * @brief 创建dynreg会话实例, 并以默认值配置会话参数
 *
 * @return void *
 * @retval 非NULL dynreg实例的句柄
 * @retval NULL   初始化失败, 一般是内存分配失败导致
 *
 */
void *qm_iot_dynreg_init(void);

/**
 * @brief 配置dynreg会话
 *
 * @param[in] handle dynreg会话句柄
 * @param[in] option 配置选项, 更多信息请参考@ref qm_iot_dynreg_option_t
 * @param[in] data   配置选项数据, 更多信息请参考@ref qm_iot_dynreg_option_t
 *
 * @return int32_t
 * @retval <QM_EOK  参数配置失败
 * @retval >=QM_EOK 参数配置成功
 *
 */
int32_t qm_iot_dynreg_setopt(void *handle, qm_iot_dynreg_option_t option, void *data);

/**
 * @brief 启动dynreg流程
 *
 * @param handle dynreg会话句柄
 *
 * @return int32_t
 * @retval <QM_EOK  数据接收失败
 * @retval >=QM_EOK 数据接收成功
 */
int32_t qm_iot_dynreg_start(void *handle);

/**
 * @brief 结束dynreg会话, 销毁实例并回收资源
 *
 * @param[in] handle 指向dynreg会话句柄的指针
 *
 * @return int32_t
 * @retval <QM_EOK  执行失败
 * @retval >=QM_EOK 执行成功
 *
 */
int32_t qm_iot_dynreg_deinit(void **handle);



#if defined(__cplusplus)
}
#endif

#endif  /* __QM_IOT_DYNREG_H__ */