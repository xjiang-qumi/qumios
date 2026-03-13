#ifndef __QM_IOT_PROV_H__
#define __QM_IOT_PROV_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include "qm_types.h"
#include "qm_iot_config.h"

/**
 * @brief provision模块收到app配网信息, 通知用户的报文内容
 */
typedef struct {
    char *ssid; 
    char *password;
    char *dynreg_host;
#if CONFIG_QM_IOT_NTP_SUPPORT
    int8_t timezone;
#endif
#if CONFIG_QM_IOT_AUDIO_URI_SUPPORT
    char *audio_uri;
#endif
} qm_iot_prov_recv_t;

/**
 * @brief dynreg模块收到从网络上来的报文时, 通知用户所调用的数据回调函数
 *
 * @param[in] handle dynreg会话句柄
 * @param[in] packet dynreg消息结构体, 存放收到的dynreg报文内容
 * @param[in] userdata 用户上下文
 *
 * @return void
 */
typedef void (* qm_iot_prov_recv_handler_t)(void *handle, const qm_iot_prov_recv_t *packet, void *userdata);


typedef enum {

    /**
     * @brief 设置回调, 它在SDK收到网络报文的时候被调用, 告知用户
     *
     * @details
     *
     * 数据类型: (qm_iot_dynreg_http_recv_handler_t)
     */
    QM_IOT_PROVDOPT_RECV_HANDLER,

        /**
     * @brief 用户需要SDK暂存的上下文
     *
     * @details
     *
     * 这个上下文指针会在 QM_IOT_PROVDOPT_RECV_HANDLER设置的回调被调用时, 由SDK传给用户
     *
     * 数据类型: (void *)
     */
    QM_IOT_PROVDOPT_USERDATA,

    QM_IOT_PROVDOPT_MAX,
} qm_iot_prov_option_t;

/**
 * @brief 创建provision会话实例, 并以默认值配置会话参数
 *
 * @return void *
 * @retval 非NULL provision实例的句柄
 * @retval NULL   初始化失败, 一般是内存分配失败导致
 * 
 */
void *qm_iot_prov_init(void);

/**
 * @brief 配置provision会话
 *
 * @param[in] handle provision会话句柄
 * @param[in] option 配置选项, 更多信息请参考@ref qm_iot_shadow_option_t
 * @param[in] data   配置选项数据, 更多信息请参考@ref qm_iot_shadow_option_t
 *
 * @return int32_t
 * @retval =QM_EOK 执行成功, <QM_EOK 执行失败
 *
 */
int32_t qm_iot_prov_setopt(void *handle, qm_iot_prov_option_t option, void *data);

/**
 * @brief 启动provision流程
 *
 * @param handle provision会话句柄
 *
 * @return int32_t
 * @retval <QM_EOK  数据接收失败
 * @retval >=QM_EOK 数据接收成功
 */

int32_t qm_iot_prov_start(void *handle);

/**
 * @brief 结束provision会话, 销毁实例并回收资源
 *
 * @param[in] handle 指向provision会话句柄的指针
 *
 * @return int32_t
 * @retval <QM_EOK  执行失败
 * @retval >=QM_EOK 执行成功
 * 
 */
int32_t qm_iot_prov_deinit(void **handle);



#if defined(__cplusplus)
}
#endif

#endif  /* __QM_IOT_PROVISION_H__ */