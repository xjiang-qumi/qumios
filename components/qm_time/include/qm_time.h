#ifndef _QM_TIME_H_
#define _QM_TIME_H_

#include "qm_types.h"
#include "qm_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_QM_TIME_SUPPORT

/** @brief Time value type (seconds since epoch). */
typedef uint64_t qm_time_t;

/**
 * @brief Time value (seconds and microseconds).
 */
typedef struct qm_timeval{
    qm_time_t tv_sec;   /**< Seconds. */
    qm_time_t tv_usec;  /**< Microseconds. */
}qm_timeval_t;

/**
 * @brief Timezone offset descriptor.
 */
typedef struct qm_timezone{
    int minuteswest;    /**< Minutes west of Greenwich. */
}qm_timezone_t;

/**
 * @brief Broken-down calendar time.
 */
struct qm_tm
{
  int tm_sec;   /**< Seconds. [0-60] (1 leap second) */
  int tm_min;  /**< Minutes. [0-59] */
  int tm_hour; /**< Hours. [0-23] */
  int tm_mday; /**< Day. [1-31] */
  int tm_mon;  /**< Month. [0-11] */
  int tm_year; /**< Year - 1900. */
  int tm_wday; /**< Day of week. [0-6] */
  int tm_yday; /**< Days in year. [0-365] */
};

/**
 * @brief Initialize time subsystem.
 * @return 0 on success, negative on failure.
 */
int qm_time_init(void);

/**
 * @brief Return the current time; optionally store in *time.
 * @param time [OUT] Optional buffer for current time.
 * @return Current time (seconds since epoch).
 */
qm_time_t qm_time(qm_time_t *time);

/**
 * @brief Get current time of day and timezone.
 * @param tv [OUT] Time value.
 * @param tz [OUT] Timezone (may be NULL).
 * @return 0 on success, negative on failure.
 */
int qm_gettimeofday(struct qm_timeval *tv, struct qm_timezone *tz);

/**
 * @brief Set current time of day and timezone.
 * @param tv [IN] Time value.
 * @param tz [IN] Timezone (may be NULL).
 * @return 0 on success, negative on failure.
 */
int qm_settimeofday(const struct qm_timeval *tv, const struct qm_timezone *tz);

/**
 * @brief Set timezone.
 * @param tz [IN] Timezone descriptor.
 * @return 0 on success, negative on failure.
 */
int qm_settimezone(const struct qm_timezone *tz);

/**
 * @brief Convert time to local broken-down time.
 * @param time [IN] Time (seconds since epoch).
 * @return Pointer to static struct qm_tm (not thread-safe).
 */
struct qm_tm *qm_localtime(const qm_time_t *time);

/**
 * @brief Convert time to local broken-down time (thread-safe).
 * @param time [IN] Time (seconds since epoch).
 * @param tm   [OUT] Buffer for result.
 * @return tm on success, NULL on failure.
 */
struct qm_tm *qm_localtime_r(const qm_time_t *time, struct qm_tm *tm);

/**
 * @brief Convert time to UTC broken-down time.
 * @param time [IN] Time (seconds since epoch).
 * @return Pointer to static struct qm_tm (not thread-safe).
 */
struct qm_tm *qm_gmtime(const qm_time_t *time);

/**
 * @brief Convert time to UTC broken-down time (thread-safe).
 * @param time [IN] Time (seconds since epoch).
 * @param tm   [OUT] Buffer for result.
 * @return tm on success, NULL on failure.
 */
struct qm_tm *qm_gmtime_r(const qm_time_t *time, struct qm_tm *tm);

/**
 * @brief Convert broken-down time to seconds since epoch; normalize tm.
 * @param tm [IN/OUT] Broken-down time.
 * @return Seconds since epoch, or (qm_time_t)-1 on error.
 */
qm_time_t qm_mktime (struct qm_tm *tm);

/**
 * @brief Format broken-down time as "Day Mon dd hh:mm:ss yyyy\n".
 * @param tm [IN] Broken-down time.
 * @return Static string (not thread-safe).
 */
char *qm_asctime(const struct qm_tm *tm);

/**
 * @brief Equivalent to asctime(localtime(time)).
 * @param time [IN] Time (seconds since epoch).
 * @return Static string (not thread-safe).
 */
char *qm_ctime(const qm_time_t *time);

/**
 * @brief Format broken-down time into buffer "Weekday Day Mon dd hh:mm:ss yyyy\n".
 * @param tm  [IN] Broken-down time.
 * @param buf [OUT] Output buffer.
 * @return buf on success, NULL on failure.
 */
char *qm_asctime_r(const struct qm_tm *tm, char *buf);

/**
 * @brief Equivalent to asctime_r(localtime_r(time, tm), buf).
 * @param time [IN] Time (seconds since epoch).
 * @param buf  [OUT] Output buffer.
 * @return buf on success, NULL on failure.
 */
char *qm_ctime_r(const qm_time_t *time, char *buf);

/**
 * @brief Return human-readable current time string (for logging).
 * @return Static string (not thread-safe).
 */
char *qm_time_print(void);

#ifdef __cplusplus
}
#endif

#endif

#endif

