/**
 * @file qm_iot_subdev.h
 * @brief subdev模块头文件, 提供子设备管理的能力
 *
 * @copyright Copyright (C) 2024 Wells. All rights reserved.
 *
 */
#ifndef __QM_IOT_DEV_H__
#define __QM_IOT_DEV_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include "qm.h"
#include "qm_iot_common.h"

#ifndef CONFIG_QM_IOT_DEV_IR_SUPPORT
#define CONFIG_QM_IOT_DEV_IR_SUPPORT           (0)
#endif

#ifndef CONFIG_QM_IOT_DEV_INFO_SUPPORT
#define CONFIG_QM_IOT_DEV_INFO_SUPPORT         (0)
#endif

typedef struct {
    uint32_t did;
    int state;
} qm_iot_dev_state_t;

/**
 * @brief dev模块收到从网络上来的报文时, 通知用户的报文类型
 */
typedef enum {
    QM_IOT_DEVRECV_NONE,
    QM_IOT_DEVRECV_IR_NOTIFY,
    QM_IOT_DEVRECV_RESET_REPLY,
    QM_IOT_DEVRECV_BIND_NOTIFY,
    QM_IOT_DEVRECV_UNBIND_NOTIFY,
    QM_IOT_DEVRECV_MAX
} qm_iot_dev_recv_type_t;

typedef struct {
    uint32_t             msg_id;
    int                  msg_len;
    union 
    {
        char             *msg;
    }data;
} qm_iot_dev_ir_notify_t;

typedef struct {
    qm_iot_code_t        code;
    uint32_t             msg_id;
} qm_iot_dev_reset_reply_t;

typedef struct {
    uint32_t            msg_id;
    union 
    {
        uint8_t         re;
    }data;
} qm_iot_dev_unbind_notify_t;

typedef struct {
    uint32_t            msg_id;
} qm_iot_dev_bind_notify_t;


/**
 * @brief dev模块收到从网络上来的报文时, 通知用户的报文内容
 */
typedef struct {
    /**
     * @brief 报文内容所对应的报文类型, 更多信息请参考@ref qm_iot_dev_recv_type_t
     */
    qm_iot_dev_recv_type_t type;
    union {

        qm_iot_dev_ir_notify_t  ir_notify;
        /**
         * @brief 从设备发起解绑请求消息后，收到的云端的应答
         */
        qm_iot_dev_reset_reply_t reset_reply;
        /**
         * @brief 收到的云端的解绑通知
         */
        qm_iot_dev_unbind_notify_t unbind_notify;
        /**
         * @brief 收到的云端的解绑通知
         */
        qm_iot_dev_bind_notify_t bind_notify;
    } data;
} qm_iot_dev_recv_t;

/**
 * @brief dev模块收到从网络上来的报文时, 通知用户所调用的数据回调函数
 *
 * @param[in] handle dev会话句柄
 * @param[in] packet dev消息结构体, 存放收到的dev报文内容
 * @param[in] userdata 用户上下文
 *
 * @return void
 */
typedef void (* qm_iot_dev_recv_handler_t)(void *handle, const qm_iot_dev_recv_t *packet, void *user_data);

/**
 * @brief @ref qm_iot_dev_setopt 接口的option参数可选值.
 *
 * @details 下文每个选项中的数据类型, 指的是@ref qm_iot_dev_setopt 中, data参数的数据类型
 *
 * 1. data的数据类型是char *时, 以配置@ref QM_IOT_SUBDEVOPT_MQTT_HANDLE 为例:
 *
 *    void *mqtt_handle = qm_iot_mqtt_init();
 *    qm_iot_dev_setopt(dev_handle, QM_IOT_DEVOPT_MQTT_HANDLE, mqtt_handle);
 */
typedef enum {
    /**
     * @brief dev会话 需要的MQTT句柄, 需要先建立MQTT连接，再设置MQTT句柄
     * 
     * @details
     * 
     * 数据类型: (void *)
     */
    QM_IOT_DEVOPT_MQTT_HANDLE,

    /**
     * @brief 设置回调, 它在SDK收到网络报文的时候被调用, 告知用户, 数据类型为(qm_iot_dev_recv_handler_t)
     * 
     * @details
     * 
     * 数据类型: (qm_iot_dev_recv_handler_t)
     */
    QM_IOT_DEVOPT_RECV_HANDLER,

    /**
     * @brief 用户需要SDK暂存的上下文, 数据类型为(void *)
     *
     * @details 这个上下文指针会在 QM_IOT_DEVOPT_RECV_HANDLER 设置的回调被调用时, 由SDK传给用户
     * 
     * 数据类型: (void *)
     */
    QM_IOT_DEVOPT_USERDATA,
    QM_IOT_DEVOPT_MAX
} qm_iot_dev_option_t;

/**
 * @brief 创建dev会话实例, 并以默认值配置会话参数
 *
 * @return void *
 * @retval 非NULL dev实例的句柄
 * @retval NULL   初始化失败, 一般是内存分配失败导致
 *
 */
void *qm_iot_dev_init(void);

/**
 * @brief 配置subdev会话
 *
 * @param[in] handle dev会话句柄
 * @param[in] option 配置选项, 更多信息请参考@ref qm_iot_subdev_option_t
 * @param[in] data   配置选项数据, 更多信息请参考@ref qm_iot_subdev_option_t
 *
 * @return int32_t
 * @retval <QM_EOK 执行失败
 * @retval >=QM_EOK 执行成功
 *
 */
int32_t qm_iot_dev_setopt(void *handle, qm_iot_dev_option_t option, void *data);

/**
 * @brief 结束subdev会话, 销毁实例并回收资源
 *
 * @param[in] handle 指向subdev会话句柄的指针
 *
 * @return int32_t
 * @retval <QM_EOK 执行失败
 * @retval >=QM_EOK 执行成功
 *
 */
int32_t qm_iot_dev_deinit(void **handle);

/**
 * @brief 向物联网平台上报设备解绑
 *
 * @param handle subdev会话句柄
 * @param subdev_state 子设备状态数组
 * @param dev_num 子设备数组中的子设备数量
 * 
 * @return int32_t
 * @retval <QM_EOK 执行失败
 * @retval >=QM_EOK 执行成功
 */
int32_t qm_iot_dev_reset_request(void *handle);

/**
 * @brief 向物联网平台上报设备信息
 *
 * @param handle dev会话句柄
 * 
 * @return int32_t
 * @retval <QM_EOK 执行失败
 * @retval >=QM_EOK 执行成功
 */
int32_t qm_iot_dev_send_devinfo(void *handle);

#if defined(__cplusplus)
}
#endif

#endif  /* __QM_IOT_SUBDEV_API_H__ */

