#ifndef _ADC_BUTTON_H_
#define _ADC_BUTTON_H_

#ifdef __cplusplus
extern "C"
{
#endif 

#include "qm_types.h"
#include "qm_config.h"

/** @brief Default ticks interval in milliseconds. */
#ifndef CONFIG_ADC_BUTTON_TICKS_INTERVAL
#define CONFIG_ADC_BUTTON_TICKS_INTERVAL 20 //ms
#endif

/** @brief Default debounce ticks count (MAX 8). */
#ifndef CONFIG_ADC_BUTTON_DEBOUNCE_TICKS
#define CONFIG_ADC_BUTTON_DEBOUNCE_TICKS 1  //MAX 8
#endif

/** @brief Default short press timeout in milliseconds. */
#ifndef CONFIG_ADC_BUTTON_SHORT_TIMEOUT
#define CONFIG_ADC_BUTTON_SHORT_TIMEOUT    200
#endif

/** @brief Default long press timeout in milliseconds. */
#ifndef CONFIG_ADC_BUTTON_LONG_TIMEOUT
#define CONFIG_ADC_BUTTON_LONG_TIMEOUT     1000
#endif

/** @brief Short press threshold in ticks. */
#define CONFIG_ADC_BUTTON_SHORT_TICKS    (CONFIG_ADC_BUTTON_SHORT_TIMEOUT / CONFIG_ADC_BUTTON_TICKS_INTERVAL)

/** @brief Long press threshold in ticks. */
#define CONFIG_ADC_BUTTON_LONG_TICKS     (CONFIG_ADC_BUTTON_LONG_TIMEOUT / CONFIG_ADC_BUTTON_TICKS_INTERVAL)

/**
 * @brief  Button event callback function type.
 */
typedef void (*button_cb)(void *handle);

/**
 * @brief  ADC value getter function type, returns the current raw ADC reading.
 */
typedef uint32_t (*adc_value_get_fun)(void);

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
    PRESS_EVENT_MAX,              /**< Number of event types (boundary). */
} press_event_t;

/**
 * @brief  ADC button handle structure.
 */
typedef struct adc_button
{
    uint16_t ticks;                    /**< Internal tick counter. */
    uint8_t repeat : 4;                /**< Repeat press counter. */
    uint8_t event : 4;                 /**< Current button event. */
    uint8_t state : 3;                 /**< State machine state. */
    uint8_t debounce_cnt : 3;          /**< Debounce counter. */
    uint8_t rsv : 1;                   /**< Reserved bit. */
    uint8_t press_down : 1;            /**< Current press state (1=pressed). */
    uint32_t adc_value_min;            /**< Minimum ADC value for press detection. */
    uint32_t adc_value_max;            /**< Maximum ADC value for press detection. */
    adc_value_get_fun adc_value_get;   /**< ADC value getter function. */
    button_cb cb[PRESS_EVENT_MAX];     /**< Callback array indexed by event. */
    struct adc_button *next;           /**< Linked list next pointer. */
} adc_button_t;

/**
 * @brief  Initialize an ADC button handle.
 *
 * @param  handle [IN/OUT] Pointer to the adc_button_t handle to initialize.
 * @param  fn [IN] Function pointer to read the current ADC value.
 * @param  adc_value_min [IN] Minimum ADC value that represents button pressed.
 * @param  adc_value_max [IN] Maximum ADC value that represents button pressed.
 */
void adc_button_init(adc_button_t *handle, adc_value_get_fun fn, uint32_t adc_value_min, uint32_t adc_value_max);

/**
 * @brief  Attach a callback function to a specific button press event.
 *
 * @param  handle [IN/OUT] Pointer to the adc_button_t handle.
 * @param  event [IN] The press event type to attach the callback to.
 * @param  cb [IN] Callback function to invoke when the event occurs.
 */
void adc_button_attach(adc_button_t *handle, press_event_t event, button_cb cb);

/**
 * @brief  Get the current button press event.
 *
 * @param  handle [IN] Pointer to the adc_button_t handle.
 *
 * @return The current press_event_t event value.
 */
press_event_t adc_button_event_get(adc_button_t *handle);

/**
 * @brief  Start the button, adding it to the internal work list.
 *
 * @param  handle [IN/OUT] Pointer to the adc_button_t handle to start.
 *
 * @return 0 on success, -1 if the handle already exists in the work list.
 */
int adc_button_start(adc_button_t *handle);

/**
 * @brief  Stop the button, removing it from the internal work list.
 *
 * @param  handle [IN/OUT] Pointer to the adc_button_t handle to stop.
 */
void adc_button_stop(adc_button_t *handle);

/**
 * @brief  Drive button state machine; must be called periodically at CONFIG_ADC_BUTTON_TICKS_INTERVAL ms.
 */
void adc_button_ticks(void);

#ifdef __cplusplus
}
#endif

#endif /* _ADC_BUTTON_H_ */
