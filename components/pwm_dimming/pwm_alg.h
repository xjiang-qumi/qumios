#ifndef _PWM_ALG_H
#define _PWM_ALG_H

/** @brief Dimming curve type: linear. */
#define DIMMING_CURVE_TYPE_LINEAR 0
/** @brief Dimming curve type: exponential. */
#define DIMMING_CURVE_TYPE_EXP    1
/** @brief Dimming curve type: logarithmic. */
#define DIMMING_CURVE_TYPE_LOG    2
/** @brief Dimming curve type: S-curve. */
#define DIMMING_CURVE_TYPE_S    3

/** @brief Exponential factor for EXP curve type. */
#define EXP_FACTOR  (2.43)

#ifndef LIGHTNESS_MAX 
/** @brief Maximum lightness value (default 10000). */
#  define LIGHTNESS_MAX 10000
#endif

/**
 * @brief Initialize the PWM dimming lookup table.
 *
 * @param curve_type  Curve type, one of DIMMING_CURVE_TYPE_*.
 * @param lightness_min  Minimum lightness value (non-zero threshold).
 */
void pwm_alg_table_init(int curve_type, int lightness_min);

/**
 * @brief Get the PWM duty-cycle value for a given lightness using EXP curve.
 *
 * @param light_id    Light channel index.
 * @param lightness   Target lightness value (0 ~ LIGHTNESS_MAX).
 * @return PWM value mapped from lightness.
 */
int pwm_alg_get_exp(int light_id, int lightness);

#endif
