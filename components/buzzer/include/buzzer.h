#ifndef _BUZZER_H_
#define _BUZZER_H_

#include "qm.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup buzzer 蜂鸣器组件
 * @{
 */

#ifndef CONFIG_BUZZER_PASSIVE_SUPPORT
#define CONFIG_BUZZER_PASSIVE_SUPPORT (0)
#endif

#ifndef CONFIG_BUZZER_ACTIVE_SUPPORT
#define CONFIG_BUZZER_ACTIVE_SUPPORT (0)
#endif

#ifndef CONFIG_BUZZER_CHRIP_INTERVAL 
#define CONFIG_BUZZER_CHRIP_INTERVAL  (500)
#endif

#if !CONFIG_BUZZER_ACTIVE_SUPPORT && !CONFIG_BUZZER_PASSIVE_SUPPORT
#error "please config active buzzer or passive buzzer"
#endif

/** @brief 蜂鸣器实例句柄。 */
typedef void* buzzer_handle_t;

/** @brief 蜂鸣器类型（有源/无源）。 */
typedef enum {
    BUZZER_TPYE_PASSIVE = 0,   /**< 无源蜂鸣器 */         
    BUZZER_TPYE_ACTIVE = 1,    /**< 有源蜂鸣器 */    
} buzzer_type_t;

/** @brief 有源蜂鸣器关闭时 GPIO 电平。 */
typedef enum {
    BUZZER_CLOSE_LOW = 0,            /**< 低电平时关闭 */
    BUZZER_CLOSE_HIGH = 1,           /**< 高电平时关闭 */
} buzzer_close_level_t;

#if CONFIG_BUZZER_PASSIVE_SUPPORT
/** @brief 无源蜂鸣器硬件参数。 */
typedef struct {
    uint8_t port;       /**< GPIO 端口 */
    uint8_t pin;        /**< GPIO 引脚 */
    int buzzer_freq;    /**< 频率 (Hz) */
    int buzzer_pulse;   /**< 占空比 (%) */
}buzzer_passive_param_t;
#endif

#if CONFIG_BUZZER_ACTIVE_SUPPORT
/** @brief 有源蜂鸣器硬件参数。 */
typedef struct {
    uint8_t port;                      /**< GPIO 端口 */
    buzzer_close_level_t close_level;  /**< 关闭时电平 */
}buzzer_active_param_t;
#endif

/** @brief 蜂鸣器创建与运行参数。 */
typedef struct {
    int count;              /**< 鸣叫次数 */
    int chirping_time;      /**< 单次鸣叫持续时间，单位 ms */
    buzzer_type_t type;     /**< 蜂鸣器类型（有源/无源） */
    union 
    {
  #if CONFIG_BUZZER_ACTIVE_SUPPORT
      buzzer_active_param_t active_buzzer;  /**< 有源蜂鸣器参数 */
  #endif
  #if CONFIG_BUZZER_PASSIVE_SUPPORT
      buzzer_passive_param_t passive_buzzer; /**< 无源蜂鸣器参数 */
  #endif
    }buzzer;   /**< 有源或无源参数，依 type 选择 */
}buzzer_param_t;

/**
  * @brief 创建蜂鸣器对象。
  *
  * @param param 蜂鸣器参数，见 @ref buzzer_param_t
  *
  * @return buzzer_handle_t 创建成功返回句柄，失败返回 NULL
  */
buzzer_handle_t buzzer_create(buzzer_param_t *param);

/**
  * @brief 启动蜂鸣器鸣叫。
  *
  * @param handle 蜂鸣器句柄，由 buzzer_create 返回
  *
  * @return
  *     - QM_EOK: 成功
  *     - 其他: 失败
  */
qm_err_t buzzer_open(buzzer_handle_t handle);

/**
  * @brief 修改蜂鸣器鸣叫参数并重新开始鸣叫。
  *
  * @param handle 蜂鸣器句柄
  * @param count 新的鸣叫次数
  * @param chirping_time 新的单次鸣叫时间 (ms)
  *
  * @return
  *     - QM_EOK: 成功
  *     - 其他: 失败
  */
qm_err_t buzzer_change(buzzer_handle_t handle, int count, int chirping_time);

/**
  * @brief 停止蜂鸣器鸣叫。
  *
  * @param handle 蜂鸣器句柄
  * @return
  *     - QM_EOK: 成功
  *     - 其他: 失败
  */
qm_err_t buzzer_close(buzzer_handle_t handle);

/**
  * @brief 删除蜂鸣器对象并释放内存。
  *
  * @param handle 蜂鸣器句柄
  *
  * @return
  *     - QM_EOK: 成功
  *     - 其他: 失败
  */
qm_err_t buzzer_delete(buzzer_handle_t handle);

/** @} */

#ifdef __cplusplus
}
#endif

#endif
