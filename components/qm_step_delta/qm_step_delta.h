#ifndef _QM_STEP_DELTA_H_
#define _QM_STEP_DELTA_H_

#include "qm.h"
#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Interpolation curve type for step-delta calculation.
 */
typedef enum {
    STEP_DELTA_CURVE_LINEAR = 0, /**< Linear interpolation. */
    STEP_DELTA_CURVE_SQUARE,     /**< Square curve (suitable for general dimming). */
    STEP_DELTA_CURVE_CUBIC,      /**< Cubic curve (finer control at low brightness). */
	STEP_DELTA_CURVE_HYBRID,     /**< Hybrid: square + linear blend. */
	STEP_DELTA_CURVE_S,          /**< S-curve. */
	STEP_DELTA_CURVE_SMOOTHERSTEP, /**< Smootherstep curve. */
} curve_type_t;

/**
 * @brief Step-delta state for incremental value interpolation.
 */
typedef struct {
    int start;          /**< Start value. */
    int end;            /**< End value. */
    int steps;          /**< Total steps to transition. */
    int current_step;   /**< Current step counter. */
    curve_type_t curve; /**< Interpolation curve type. */
	int32_t error_acc;  /**< Error accumulator for fractional steps. */
} qm_step_delta_t;

/**
 * @brief Initialize a step-delta transition.
 *
 * @param dest      Pointer to step-delta state.
 * @param start     Start value.
 * @param end       End value.
 * @param step_cnt  Number of steps.
 * @param curve     Interpolation curve type.
 * @return 0 on success.
 */
int qm_step_delta_init(qm_step_delta_t *dest, int start, int end, int step_cnt, curve_type_t curve);

/**
 * @brief Advance to the next step and return the interpolated value.
 *
 * @param dest  Pointer to step-delta state.
 * @return Interpolated value at the current step.
 */
int qm_step_delta_get_next(qm_step_delta_t *dest);

/**
 * @brief Get the final (end) value.
 *
 * @param dest  Pointer to step-delta state.
 * @return End value.
 */
int qm_step_delta_get_end(qm_step_delta_t *dest);

/**
 * @brief Get the number of remaining steps.
 *
 * @param dest  Pointer to step-delta state.
 * @return Remaining step count.
 */
int qm_step_delta_left_steps(qm_step_delta_t *dest);

#ifdef __cplusplus
}
#endif

#endif
