#ifndef _MULTI_BUTTON_H_
#define _MULTI_BUTTON_H_

#ifdef __cplusplus
extern "C"
{
#endif 

#include "qm_types.h"
#include "qm_config.h"

#ifndef CONFIG_MULTI_BUTTON_TICKS_INTERVAL
#define CONFIG_MULTI_BUTTON_TICKS_INTERVAL 20 //ms
#endif

#ifndef CONFIG_MULTI_BUTTON_DEBOUNCE_TICKS
#define CONFIG_MULTI_BUTTON_DEBOUNCE_TICKS 1  //MAX 8
#endif

#ifndef CONFIG_MULTI_BUTTON_SHORT_TIMEOUT
#define CONFIG_MULTI_BUTTON_SHORT_TIMEOUT    200
#endif

#ifndef CONFIG_MULTI_BUTTON_LONG_TIMEOUT
#define CONFIG_MULTI_BUTTON_LONG_TIMEOUT     1000
#endif

#define CONFIG_MULTI_BUTTON_SHORT_TICKS    (CONFIG_MULTI_BUTTON_SHORT_TIMEOUT / CONFIG_MULTI_BUTTON_TICKS_INTERVAL)
#define CONFIG_MULTI_BUTTON_LONG_TICKS     (CONFIG_MULTI_BUTTON_LONG_TIMEOUT / CONFIG_MULTI_BUTTON_TICKS_INTERVAL)

/**
 * @brief  Button event callback function type.
 */
typedef void (*button_cb)(void *handle);

/**
 * @brief  GPIO pin level read function type.
 */
typedef uint8_t (*pin_level_get_fn)(void);

/**
 * @brief  Button press event types.
 */
typedef enum
{
    PRESS_EVENT_NONE = 0,        /**< No event. */
    PRESS_EVENT_DOWN,            /**< Button pressed down. */
    PRESS_EVENT_UP,              /**< Button released. */
    PRESS_EVENT_REPEAT,          /**< Button held and repeating. */
    PRESS_EVENT_SINGLE_CLICK,    /**< Single click detected. */
    PRESS_EVENT_DOUBLE_CLICK,    /**< Double click detected. */
    PRESS_EVENT_LONG_START,      /**< Long press started. */
    PRESS_EVENT_LONG_HOLD,       /**< Long press holding. */
    PRESS_EVENT_MAX,             /**< Number of event types (boundary). */
} press_event_t;

/**
 * @brief  Multi-button handle structure.
 */
typedef struct multi_button
{
    uint16_t ticks;               /**< Tick counter. */
    uint8_t repeat : 4;           /**< Repeat count. */
    uint8_t event : 4;            /**< Current event. */
    uint8_t state : 3;            /**< State machine state. */
    uint8_t debounce_cnt : 3;     /**< Debounce counter. */
    uint8_t active_level : 1;     /**< Active level (0=low, 1=high). */
    uint8_t button_level : 1;     /**< Current button level. */
    pin_level_get_fn pin_level_get; /**< Function pointer to read pin level. */
    button_cb cb[PRESS_EVENT_MAX];  /**< Callbacks for each event type. */
    struct multi_button *next;      /**< Next button in the linked list. */
} multi_button_t;

/**
 * @brief  Initialize a multi-button handle.
 * @param  handle [IN/OUT] Pointer to the multi_button_t handle to initialize.
 * @param  fn [IN] Function pointer to read the GPIO pin level.
 * @param  active_level [IN] Active level of the button (0 or 1).
 */
void multi_button_init(multi_button_t *handle, pin_level_get_fn fn, uint8_t active_level);

/**
 * @brief  Attach a callback to a button event.
 * @param  handle [IN] Pointer to the multi_button_t handle.
 * @param  event [IN] Event type to attach the callback to.
 * @param  cb [IN] Callback function to invoke on the event.
 */
void multi_button_attach(multi_button_t *handle, press_event_t event, button_cb cb);

/**
 * @brief  Get the current event of the button.
 * @param  handle [IN] Pointer to the multi_button_t handle.
 * @return Current press event type.
 */
press_event_t multi_button_event_get(multi_button_t *handle);

/**
 * @brief  Start monitoring the button.
 * @param  handle [IN] Pointer to the multi_button_t handle.
 * @return 0 on success, negative value on failure.
 */
int multi_button_start(multi_button_t *handle);

/**
 * @brief  Stop monitoring the button.
 * @param  handle [IN] Pointer to the multi_button_t handle.
 */
void multi_button_stop(multi_button_t *handle);

/**
 * @brief  Drive button state machine; call periodically at the configured tick interval.
 */
void multi_button_ticks(void);

#ifdef __cplusplus
}
#endif

#endif
