#ifndef __QM_WORK_QUEUE_H__
#define __QM_WORK_QUEUE_H__

#include "qm_kernel.h"
#include "qm_types.h"
#include "qm_config.h"

#ifndef CONFIG_QM_WORKQUEUE_TASK_SIZE
#define CONFIG_QM_WORKQUEUE_TASK_SIZE     5*1024
#endif 

#ifndef CONFIG_QM_WORKQUEUE_TASK_PRIO
#define CONFIG_QM_WORKQUEUE_TASK_PRIO     20
#endif  

#ifndef CONFIG_QM_WORKQUEUE_NUM
#define CONFIG_QM_WORKQUEUE_NUM       10
#endif   


#ifndef CONFIG_QM_WORKQUEUE_NO_TASK
#define CONFIG_QM_WORKQUEUE_NO_TASK   0
#endif


typedef void (*work_fn_t)(void *arg);

typedef struct {
    void *arg;
    work_fn_t fn;
    uint32_t dly;
    qm_timer_t timer;
    void *wq;
}qm_work_t;

typedef struct {
    qm_queue_t queue;
    qm_task_t worker;
}qm_workqueue_t;


/**
 * This function will creat a workqueue.
 *
 * @param[in]  workqueue   the workqueue to be created.
 * @param[in]  pri         the priority of the worker.
 * @param[in]  stack_size  the size of the worker-stack.
 *
 * @return  0: success.
 */
int32_t qm_workqueue_create(qm_workqueue_t *workqueue, int pri, int stack_size);

/**
 * This function will initialize a work.
 *
 * @param[in]  work  the work to be initialized.
 * @param[in]  fn    the call back function to run.
 * @param[in]  arg   the paraments of the function.
 * @param[in]  dly   ms to delay before run.
 *
 * @return  0: success.
 */
int32_t qm_work_init(qm_work_t *work, void (*fn)(void *), void *arg, uint32_t dly);

/**
 * This function will destroy a work.
 *
 * @param[in]  work  the work to be destroied.
 */
int32_t qm_work_cancel(qm_work_t *work);

/**
 * This function will run a work on a workqueue.
 *
 * @param[in]  workqueue  the workqueue to run work.
 * @param[in]  work       the work to run.
 *
 * @return  0: success.
 */
int32_t qm_work_sched(qm_workqueue_t *workqueue, qm_work_t *work);

/**
 * This function will run a work on the default workqueue.
 *
 * @param[in]  work  the work to run.
 *
 * @return  0: success.
 */
int32_t qm_work_on_sched(qm_work_t *work);

/**
 * This function will cancel a work on the default workqueue.
 *
 * @param[in]  work  the work to cancel.
 *
 * @return  0: success.
 */
int32_t qm_work_on_cancel(qm_work_t *work);

/**
 * This function init workqueue on the default workqueue.
 *
 * @param[in]  pri         the priority of the worker.
 * @param[in]  stack_size  the size of the worker-stack.
 *
 * @return  0: success.
 */
int32_t qm_work_on_init(void);

#if CONFIG_QM_WORKQUEUE_NO_TASK
/**
 * @brief 当workqueue不使用task时，需要调用此函数来处理workqueue
 * 
 */
void qm_workqueue_process(void);
#endif

#endif

