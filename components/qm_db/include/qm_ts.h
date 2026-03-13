#ifndef QM_TS_H
#define QM_TS_H

#ifdef __cplusplus
extern "C"{
#endif

#include "qm_errno.h"
#include "qm_types.h"

#define QM_TS_STATUS_NUM      6

/**
 * @brief Time-series node status.
 */
typedef enum qm_ts_status {
    QM_TS_UNUSED,        /**< Unused. */
    QM_TS_PRE_WRITE,     /**< Pre-write. */
    QM_TS_WRITE,         /**< Written. */
    QM_TS_USER_STATUS1,  /**< User status 1. */
    QM_TS_DELETED,       /**< Deleted. */
    QM_TS_USER_STATUS2,  /**< User status 2. */
}qm_ts_status_t;

/**
 * @brief Time-series node descriptor.
 */
typedef struct qm_ts{
    qm_ts_status_t status;      /**< node status, @see qm_ts_status_t */
    uint32_t time;              /**< node timestamp */
    uint32_t len;               /**< data length */
    uint32_t addr;              /**< node address */
    uint32_t index;             /**< node index address */
}qm_ts_t;

/**
 * @brief Callback type for time-series iteration.
 * @param ts  Node descriptor.
 * @param arg User argument.
 * @return true to continue, false to stop.
 */
typedef bool_t (*qm_ts_cb)(qm_ts_t *ts, void *arg);
/** @brief Function type to get current timestamp. */
typedef uint32_t (*qm_ts_get_time)(void);

/**
 * @brief Initialize time-series store.
 * @param name     Store name.
 * @param get_time Callback to get current time.
 * @param max_len  Max data length per node.
 * @return Opaque handle on success, NULL on failure.
 */
void *qm_ts_init(const char *name, qm_ts_get_time get_time, uint32_t max_len);
/**
 * @brief Destroy time-series store and free resources.
 * @param db Handle from qm_ts_init.
 * @return 0 on success, negative on failure.
 */
int qm_ts_clean(void *db);

/**
 * @brief Append a record to the time-series store.
 * @param db  Handle from qm_ts_init.
 * @param buf Data to append.
 * @param len Length of buf.
 * @return 0 on success, negative on failure.
 */
int qm_ts_append(void *db, const void *buf, uint32_t len);
/**
 * @brief Read node data into buffer.
 * @param db  Handle from qm_ts_init.
 * @param ts  Node descriptor (filled by iterator).
 * @param buf Output buffer.
 * @param len Buffer length.
 * @return 0 on success, negative on failure.
 */
int qm_ts_read(void *db, qm_ts_t *ts, void *buf, uint32_t len);

/**
 * @brief Iterate nodes in forward order.
 * @param db  Handle from qm_ts_init.
 * @param cb  Callback per node.
 * @param arg User argument for cb.
 * @return 0 on success, negative on failure.
 */
int qm_ts_iter(void *db, qm_ts_cb cb, void *arg);
/**
 * @brief Iterate nodes in reverse order.
 * @param db  Handle from qm_ts_init.
 * @param cb  Callback per node.
 * @param arg User argument for cb.
 * @return 0 on success, negative on failure.
 */
int qm_ts_iter_reverse(void *db, qm_ts_cb cb, void *arg);
/**
 * @brief Iterate nodes with given status (forward).
 * @param db     Handle from qm_ts_init.
 * @param status Filter by status.
 * @param cb     Callback per node.
 * @param arg    User argument for cb.
 * @return 0 on success, negative on failure.
 */
int qm_ts_iter_by_status(void *db, qm_ts_status_t status, qm_ts_cb cb, void *arg);
/**
 * @brief Iterate nodes with given status (reverse).
 * @param db     Handle from qm_ts_init.
 * @param status Filter by status.
 * @param cb     Callback per node.
 * @param arg    User argument for cb.
 * @return 0 on success, negative on failure.
 */
int qm_ts_iter_reverse_by_status(void *db, qm_ts_status_t status, qm_ts_cb cb, void *arg);
/**
 * @brief Iterate nodes in time range (forward).
 * @param db    Handle from qm_ts_init.
 * @param from  Start time (inclusive).
 * @param to    End time (inclusive).
 * @param cb    Callback per node.
 * @param arg   User argument for cb.
 * @return 0 on success, negative on failure.
 */
int qm_ts_iter_by_time(void *db, uint32_t from, uint32_t to, qm_ts_cb cb, void *arg);
/**
 * @brief Iterate nodes in time range with status filter (forward).
 * @param db     Handle from qm_ts_init.
 * @param from   Start time (inclusive).
 * @param to     End time (inclusive).
 * @param status Filter by status.
 * @param cb     Callback per node.
 * @param arg    User argument for cb.
 * @return 0 on success, negative on failure.
 */
int qm_ts_iter_by_time_and_status(void *db, uint32_t from, uint32_t to, qm_ts_status_t status, qm_ts_cb cb, void *arg);
/**
 * @brief Count nodes in time range with given status.
 * @param db     Handle from qm_ts_init.
 * @param from   Start time (inclusive).
 * @param to     End time (inclusive).
 * @param status Filter by status.
 * @return Count on success, negative on failure.
 */
int qm_ts_query_count(void *db, uint32_t from, uint32_t to, qm_ts_status_t status);
/**
 * @brief Set status of a node.
 * @param db     Handle from qm_ts_init.
 * @param ts     Node descriptor.
 * @param status New status.
 * @return 0 on success, negative on failure.
 */
int qm_ts_set_status(void *db, qm_ts_t *ts, qm_ts_status_t status);



#ifdef __cplusplus
}
#endif

#endif /* QM_TS_H */
