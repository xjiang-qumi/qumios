#ifndef _LED_BREATH_LED_H
#define _LED_BREATH_LED_H

#ifdef __cplusplus
extern "C" {
#endif

#include "qm.h"

#ifndef LED_BREATH_PWM_CHANNAL_MAX
#define LED_BREATH_PWM_CHANNAL_MAX     5
#endif

#ifndef LED_BREATH_MAX_ACTION_NUM
#define LED_BREATH_MAX_ACTION_NUM      5
#endif
/* timer interval, minimum value is 10ms */
#ifndef LED_BREATH_MONITOR_INTERVAL
#define LED_BREATH_MONITOR_INTERVAL   20
#endif

#ifndef LED_BREATH_PWM_LIGHT_FREQUENCY
#define LED_BREATH_PWM_LIGHT_FREQUENCY 1000
#endif

#ifndef LED_BREATH_PWM_MAX_PULSE
#define LED_BREATH_PWM_MAX_PULSE      (10000)
#endif

/**
 * @brief  Initialize the breath LED controller.
 */
void led_breath_controller_init(void);

/**
 * @brief  Deinitialize the breath LED on the given GPIO pin.
 * @param  gpio_pin [IN] GPIO pin number.
 * @return QM_EOK on success, error code on failure.
 */
qm_err_t led_breath_deinit(int gpio_pin);

/**
 * @brief  Initialize breath LED on the given GPIO pin.
 * @param  gpio_pin [IN] GPIO pin number.
 * @param  breath_time [IN] Breathing cycle time in milliseconds.
 * @return QM_EOK on success, error code on failure.
 */
qm_err_t led_breath_init(int gpio_pin, int breath_time);

/**
 * @brief  Start the breath LED animation.
 * @param  gpio_pin [IN] GPIO pin number.
 * @return QM_EOK on success, error code on failure.
 */
qm_err_t led_breath_start(int gpio_pin);

/**
 * @brief  Stop the breath LED animation.
 * @param  gpio_pin [IN] GPIO pin number.
 * @return QM_EOK on success, error code on failure.
 */
qm_err_t led_breath_stop(int gpio_pin);

/**
 * @brief  Start breath LED in fast mode with specified enable level.
 * @param  gpio_pin [IN] GPIO pin number.
 * @param  en_level [IN] Enable level for fast start.
 * @return QM_EOK on success, error code on failure.
 */
qm_err_t led_breath_fast_start(int gpio_pin, uint16_t en_level);

#ifdef __cplusplus
}
#endif

#endif