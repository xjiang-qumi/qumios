#ifndef _LIGHT_DELAY_ACTION_H
#define _LIGHT_DELAY_ACTION_H

#include "stdint.h"
#include "qm_eventbus_static.h"

/** @brief Generic void callback wrapper. */
typedef struct {
	void (*callback)(void *arg); /**< Callback function pointer. */
	void *arg;                   /**< Argument passed to callback. */
}qm_void_callback_t;

/** @brief Delay action entry stored in the event bus. */
typedef struct {
	uint8_t type;           /**< Action type: DELAYACTION_TYPE_FN or DELAYACTION_TYPE_EV. */
	uint8_t repeat;         /**< Repeat count (0 = once). */
	uint8_t busid;          /**< Event bus ID. */
	uint8_t id;             /**< Action ID (DELAYACTION_INVALID_ID if unused). */
	uint32_t time_ms;       /**< Delay duration in milliseconds. */
	uint32_t current_ms;    /**< Elapsed time counter. */
	union {
		qm_void_callback_t fn; /**< Callback when type == DELAYACTION_TYPE_FN. */
	} data;
}qm_eventbus_delayaction_t;


/** @brief Delay action type: function callback. */
#define DELAYACTION_TYPE_FN 1
/** @brief Delay action type: event post. */
#define DELAYACTION_TYPE_EV 2
/** @brief Invalid action ID sentinel. */
#define DELAYACTION_INVALID_ID 0

/**
 * @brief Initialize the delay action module.
 * @return 0 on success, negative on failure.
 */
int qm_eventbus_delay_action_init(void);

/**
 * @brief Deinitialize the delay action module.
 * @return 0 on success, negative on failure.
 */
int qm_eventbus_delay_action_deinit(void);

/**
 * @brief Schedule a function callback after a delay.
 *
 * @param busid     Event bus ID.
 * @param delay_ms  Delay in milliseconds.
 * @param repeat    Number of additional repeats (0 = fire once).
 * @param fn        Callback to invoke.
 * @return 0 on success, negative on failure.
 */
int qm_eventbus_delay_fn_start(int busid, uint32_t delay_ms, uint8_t repeat, qm_void_callback_t *fn);

/**
 * @brief Cancel a scheduled function callback.
 *
 * @param busid  Event bus ID.
 * @param fn     Callback previously registered.
 * @return 0 on success, negative on failure.
 */
int qm_eventbus_delay_fn_stop(int busid, qm_void_callback_t *fn);

/**
 * @brief Process a pending delay action event (called from event handler).
 *
 * @param busid  Event bus ID.
 * @param ev     Pointer to event.
 * @return 0 on success, negative on failure.
 */
int qm_eventbus_delay_action_handler(int busid, qm_eventbus_event_t *ev);


#endif
