#ifndef _CRAZY_BUTTON_H_
#define _CRAZY_BUTTON_H_

#ifdef __cplusplus
extern "C"
{
#endif 

#include "qm_types.h"
#include "qm_config.h"

/** @brief Default ticks interval in milliseconds. */
#ifndef CONFIG_CRAZY_BUTTON_TICKS_INTERVAL
#define CONFIG_CRAZY_BUTTON_TICKS_INTERVAL 20 //ms
#endif

/** @brief Default debounce ticks count (MAX 8). */
#ifndef CONFIG_CRAZY_BUTTON_DEBOUNCE_TICKS
#define CONFIG_CRAZY_BUTTON_DEBOUNCE_TICKS 1  //MAX 8
#endif

/** @brief Default short press timeout in milliseconds (consecutive click window). */
#ifndef CONFIG_CRAZY_BUTTON_SHORT_TIMEOUT
#define CONFIG_CRAZY_BUTTON_SHORT_TIMEOUT    500 //连续单击的超时时间，超过就重新计数
#endif

/** @brief Default repeat operation timeout in milliseconds (between consecutive operations). */
#ifndef CONFIG_CRAZY_BUTTON_REPEAT_TIMEOUT
#define CONFIG_CRAZY_BUTTON_REPEAT_TIMEOUT   400 //连续操作之间的超时时间，比如单击之后，立马长按，这两个操作之间的超时时间
#endif

/** @brief Default long press timeout in milliseconds. */
#ifndef CONFIG_CRAZY_BUTTON_LONG_TIMEOUT
#define CONFIG_CRAZY_BUTTON_LONG_TIMEOUT     1000
#endif

/** @brief Default long press report interval in milliseconds. */
#ifndef CONFIG_CRAZY_BUTTON_LONG_REPORT_TIME_MS
#define CONFIG_CRAZY_BUTTON_LONG_REPORT_TIME_MS 1000 //长按上报的时间间隔, 默认长按时，每秒报一次
#endif

/** @brief Timeout in milliseconds to clear previous button state (last_event, last_repeat). */
#ifndef CONFIG_CRAZY_BUTTON_LAST_TIMEOUT
#define CONFIG_CRAZY_BUTTON_LAST_TIMEOUT         1000  //超过这个时间，清除之前的按键状态(last_event, last_repeat)
#endif

/** @brief Short press threshold in ticks. */
#define CONFIG_CRAZY_BUTTON_SHORT_TICKS    (CONFIG_CRAZY_BUTTON_SHORT_TIMEOUT / CONFIG_CRAZY_BUTTON_TICKS_INTERVAL)
/** @brief Long press threshold in ticks. */
#define CONFIG_CRAZY_BUTTON_LONG_TICKS     (CONFIG_CRAZY_BUTTON_LONG_TIMEOUT / CONFIG_CRAZY_BUTTON_TICKS_INTERVAL)
/** @brief Long press report threshold in ticks. */
#define CONFIG_CRAZY_BUTTON_LONG_REPORT_TICKS (CONFIG_CRAZY_BUTTON_LONG_REPORT_TIME_MS / CONFIG_CRAZY_BUTTON_TICKS_INTERVAL)
/** @brief Last state clear threshold in ticks. */
#define CONFIG_CRAZY_BUTTON_LAST_TICKS   (CONFIG_CRAZY_BUTTON_LAST_TIMEOUT / CONFIG_CRAZY_BUTTON_TICKS_INTERVAL)

/**
 * @brief  Button event callback function type.
 */
typedef void (*button_cb)(void *handle);

/**
 * @brief  GPIO level getter function type, returns the current pin level.
 */
typedef uint8_t (*pin_level_get_fn)(void);

/**
 * @brief  Button press event types.
 */
typedef enum
{
    PRESS_EVENT_NONE = 0,         /**< No event. */
    PRESS_EVENT_DOWN,             /**< Button pressed down. */
    PRESS_EVENT_UP,               /**< Button released. */
    PRESS_EVENT_REPEAT,           /**< Button repeat press. */
    PRESS_EVENT_SINGLE_CLICK,     /**< Single click detected. */
    PRESS_EVENT_DOUBLE_CLICK,     /**< Double click detected. */
    PRESS_EVENT_LONG_START,       /**< Long press started. */
    PRESS_EVENT_LONG_HOLD,        /**< Long press holding. */
    PRESS_EVENT_LONG_RELEASE,     /**< Long press released. */
    PRESS_EVENT_MAX,              /**< Number of event types (boundary). */
} press_event_t;

/**
 * @brief  Crazy button handle structure.
 */
typedef struct crazy_button
{
    uint8_t repeat : 4;             /**< Current repeat press count. */
    uint8_t event : 4;              /**< Current button event. */
    uint8_t last_repeat : 4;        /**< Previous repeat press count. */
    uint8_t last_event : 4;         /**< Previous button event. */
    uint8_t state : 3;              /**< State machine state. */
    uint8_t debounce_cnt : 3;       /**< Debounce counter. */
    uint8_t active_level : 1;       /**< Active level (0=low active, 1=high active). */
    uint8_t button_level : 1;       /**< Current debounced button level. */
    uint16_t ticks;                 /**< Current tick counter. */
    uint16_t last_ticks;            /**< Tick counter for last state timeout. */
    pin_level_get_fn pin_level_get; /**< GPIO level getter function. */
    button_cb cb;                   /**< Unified event callback. */
    struct crazy_button *next;      /**< Linked list next pointer. */
} crazy_button_t;

/**
 * @brief  Initialize a crazy button handle.
 *
 * @param  handle [IN/OUT] Pointer to the crazy_button_t handle to initialize.
 * @param  fn [IN] Function pointer to read the current GPIO pin level.
 * @param  active_level [IN] Active level that represents button pressed (0 or 1).
 */
void crazy_button_init(crazy_button_t *handle, pin_level_get_fn fn, uint8_t active_level);

/**
 * @brief  Attach a unified callback for all button events.
 *
 * @param  handle [IN/OUT] Pointer to the crazy_button_t handle.
 * @param  cb [IN] Callback function invoked on any button event.
 */
void crazy_button_attach(crazy_button_t *handle, button_cb cb);

/**
 * @brief  Get the current button press event.
 *
 * @param  handle [IN] Pointer to the crazy_button_t handle.
 *
 * @return The current press_event_t event value.
 */
press_event_t crazy_button_event_get(crazy_button_t *handle);

/**
 * @brief  Start the button, adding it to the internal work list.
 *
 * @param  handle [IN/OUT] Pointer to the crazy_button_t handle to start.
 *
 * @return 0 on success, -1 if the handle already exists in the work list.
 */
int crazy_button_start(crazy_button_t *handle);

/**
 * @brief  Stop the button, removing it from the internal work list.
 *
 * @param  handle [IN/OUT] Pointer to the crazy_button_t handle to stop.
 */
void crazy_button_stop(crazy_button_t *handle);

/**
 * @brief  Drive button state machine; must be called periodically at CONFIG_CRAZY_BUTTON_TICKS_INTERVAL ms.
 */
void crazy_button_ticks(void);

/**
 * @brief  Convert tick count to milliseconds.
 *
 * @param  ticks [IN] Tick count to convert.
 *
 * @return Equivalent time in milliseconds.
 */
int crazy_button_ticks_to_ms(uint16_t ticks);

#ifdef __cplusplus
}
#endif

#endif
