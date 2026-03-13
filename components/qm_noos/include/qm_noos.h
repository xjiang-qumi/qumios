
#ifndef _QM_NOOS_H_
#define _QM_NOOS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "qm_config.h"
#include "qm_types.h"

#if CONFIG_QM_NOOS_SUPPORT

/** @brief Queue type: base. */
#define QM_NOOS_QUEUE_TYPE_BASE         0
/** @brief Queue type: semaphore. */
#define QM_NOOS_QUEUE_TYPE_SEMAPHORE    1

/**
 * @brief Initialize the no-OS (bare-metal) layer.
 * @return 0 on success, negative on failure.
 */
int qm_noos_init(void);
/**
 * @brief Create a generic queue.
 * @param queue      [OUT] Receives the new queue handle.
 * @param type       Queue type (QM_NOOS_QUEUE_TYPE_*).
 * @param queue_len  Max number of messages.
 * @param queue_size Size of each message in bytes.
 * @return 0 on success, negative on failure.
 */
int qm_noos_generic_queue_new(void **queue, uint8_t type, uint32_t queue_len, uint32_t queue_size);
/**
 * @brief Free a queue and its resources.
 * @param queue Queue handle from qm_noos_generic_queue_new.
 * @return 0 on success, negative on failure.
 */
int qm_noos_generic_queue_free(void *queue);
/**
 * @brief Send a message to the queue.
 * @param queue Queue handle.
 * @param msg   Message buffer.
 * @param size  Message size in bytes.
 * @return 0 on success, negative on failure.
 */
int qm_noos_generic_queue_send(void *queue, void *msg, uint32_t size);
/**
 * @brief Receive a message from the queue.
 * @param queue Queue handle.
 * @param msg   [OUT] Buffer for message.
 * @param size  [IN/OUT] In: buffer size; out: received size.
 * @return 0 on success, negative on failure.
 */
int qm_noos_generic_queue_recv(void *queue, void *msg, uint32_t *size);
/**
 * @brief Check if queue handle is valid.
 * @param queue Queue handle.
 * @return true if valid, false otherwise.
 */
bool_t qm_noos_generic_queue_is_valid(void *queue);
/**
 * @brief Check if queue has no messages.
 * @param queue Queue handle.
 * @return true if empty, false otherwise.
 */
bool_t qm_noos_generic_queue_is_empty(void *queue);

/**
 * @brief Create a new task.
 * @param task [OUT] Receives the new task handle.
 * @param name Task name.
 * @param fn   Entry function.
 * @param arg  Argument passed to fn.
 * @param prio Priority.
 * @return 0 on success, negative on failure.
 */
int qm_noos_task_new(void **task, const char *name, void (*fn)(void *), void *arg, int prio);
/**
 * @brief Exit and clean up a task.
 * @param task Task handle from qm_noos_task_new.
 * @return 0 on success, negative on failure.
 */
int qm_noos_task_exit(void *task);


#endif

#ifdef __cplusplus
}
#endif

#endif /* QM_NOOS_H */

