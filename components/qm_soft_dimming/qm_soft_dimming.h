#ifndef QM_SOFT_DIMMING_H_
#define QM_SOFT_DIMMING_H_

#include <stdint.h>
#include <stdbool.h>

/** @brief Maximum number of dimming channels supported. */
#ifndef QM_SOFT_DIM_MAX_CHANNELS
#define QM_SOFT_DIM_MAX_CHANNELS   5
#endif

#ifndef SOFT_DIM_MAX_CHANNELS
#define SOFT_DIM_MAX_CHANNELS QM_SOFT_DIM_MAX_CHANNELS
#endif

/**
 * @brief Configuration for soft dimming initialization.
 */
typedef struct {
    uint8_t  channel_count;    /**< Number of active channels. */
    uint32_t sw_resolution;    /**< Software resolution (steps). */
    uint32_t hw_resolution;    /**< Hardware PWM resolution (steps). */
    uint32_t damping_coef;     /**< Damping coefficient. */
    float    max_speed_pct;    /**< Max speed as percentage. */
    uint32_t timer_base_ms;    /**< Timer base interval in ms. */
} qm_softdim_config_t;

/**
 * @brief Runtime handle for a soft dimming instance.
 */
typedef struct {
    uint16_t curr[QM_SOFT_DIM_MAX_CHANNELS];  /**< Current value per channel. */
    uint16_t dest[QM_SOFT_DIM_MAX_CHANNELS];  /**< Target value per channel. */

    uint32_t curr_fixed[SOFT_DIM_MAX_CHANNELS]; /**< Q.2 fixed-point current value. */

    uint8_t  active_ch_count;   /**< Active channel count. */
    uint32_t sw_res;            /**< Software resolution. */
    uint32_t curve_divisor;     /**< Curve divisor. */

    uint32_t min_step_fixed;    /**< Minimum step in fixed-point. */
    uint32_t max_step_fixed;    /**< Maximum step in fixed-point. */

    uint32_t timer_period;      /**< Timer period in ms. */
    uint32_t phys_total_steps;  /**< Physical total steps. */
    uint32_t tick_interval;     /**< Tick interval. */
    uint32_t tick_acc;          /**< Tick accumulator. */
    uint32_t prev_step_fixed;   /**< Previous step in fixed-point. */
} qm_softdim_handle_t;

/**
 * @brief Initialize a soft dimming handle with configuration.
 *
 * @param handle  Pointer to soft dimming handle.
 * @param config  Pointer to configuration.
 */
void qm_softdim_init(qm_softdim_handle_t *handle, const qm_softdim_config_t *config);

/**
 * @brief Set the target value for a single channel.
 *
 * @param handle   Pointer to soft dimming handle.
 * @param ch_idx   Channel index.
 * @param val      Target value.
 * @param fade_ms  Fade duration in milliseconds.
 */
void qm_softdim_setTarget(qm_softdim_handle_t *handle, uint8_t ch_idx, uint16_t val, uint32_t fade_ms);

/**
 * @brief Set the same target value for all channels.
 *
 * @param handle   Pointer to soft dimming handle.
 * @param val      Target value.
 * @param fade_ms  Fade duration in milliseconds.
 */
void qm_softdim_setAll(qm_softdim_handle_t *handle, uint16_t val, uint32_t fade_ms);

/**
 * @brief Get the current value of a channel.
 *
 * @param handle  Pointer to soft dimming handle.
 * @param ch_idx  Channel index.
 * @return Current value.
 */
uint16_t qm_softdim_getCurrent(qm_softdim_handle_t *handle, uint8_t ch_idx);

/**
 * @brief Advance one dimming tick; returns true if any channel is still transitioning.
 *
 * @param handle  Pointer to soft dimming handle.
 * @return true if still active, false if all channels have reached their targets.
 */
bool qm_softdim_run(qm_softdim_handle_t *handle);

/**
 * @brief Check whether any channel is actively transitioning.
 *
 * @param handle  Pointer to soft dimming handle.
 * @return true if active, false if idle.
 */
bool qm_softdim_isActive(qm_softdim_handle_t *handle);

#endif
