#ifndef __QM_WORK_H__
#define __QM_WORK_H__

#include "qm_kernel.h"
#include "qm_types.h"
#include "qm_workqueue.h"
#include "qm_eventqueue.h"
#include "qm_config.h"

enum qm_queue_type
{
    QM_QUEUE_EVENT_TYPE,
    QM_QUEUE_WORK_TYPE
};

typedef struct{
    uint8_t queue_type;
    union {
        qm_work_t *work;
        qm_input_event_t event;
    }info;
}qm_queue_info_t;

/**
 * Post a delayed action to be executed in main loop.
 *
 * @param[in]  work    handle of work
 * @param[in]  action  action to be executed.
 * @param[in]  arg     private data past to action.
 * @param[in]  ms      milliseconds to wait.
 *
 * @return  the operation status, 0 is OK,others is error.
 */
int32_t qm_post_delayed_action(qm_work_t *work, void (*fn)(void *), void *arg, uint32_t ms);

/**
 * Cancel a delayed action to be executed in main loop.
 *
 * @param[in]  work  handle of work
 * 
 * @return  the operation status, 0 is OK,others is error.
 */
int32_t qm_cancel_delayed_action(qm_work_t *work);


#endif
