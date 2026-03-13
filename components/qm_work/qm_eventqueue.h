#ifndef __QM_EVENT_QUEUE_H__
#define __QM_EVENT_QUEUE_H__

#include "qm_types.h"
#include "qm_config.h"

#ifndef CONFIG_QM_EVENT_NUM 
#define CONFIG_QM_EVENT_NUM    16
#endif   


typedef struct {
    /* The time event is generated, auto filled */
    uint32_t time;
    /* Event type, value < 0x1000 are used by qmos system */
    uint16_t event;
    /* Defined according to event */
    uint16_t sub_event;
    /* Defined according to value */
    void *value;
    /* Defined the size of value */
    uint32_t size;

    void *arg;
} qm_input_event_t;

/* Event callback */
typedef void (*qm_event_cb)(qm_input_event_t *input_event, void *arg);


/**
 * Register system event filter callback.
 *
 * @param[in]  event  event type interested.
 * @param[in]  cb    system event callback.
 * @param[in]  arg   private data past to cb.
 *
 * @return  the operation status, 0 is OK, others is error.
 */
int32_t qm_event_register(uint16_t event, qm_event_cb cb, void *arg);

/**
 * Unregister native event callback.
 *
 * @param[in]  event  event type interested.
 * @param[in]  cb    system event callback.
 * @param[in]  arg   private data past to cb.
 *
 * @return  the operation status, 0 is OK, others is error.
 */
int32_t qm_event_unregister(uint16_t event, qm_event_cb cb, void *arg);

/**
 * @brief Posts an event to the specified event loop. The event loop library keeps a copy of event_data and manages
 * the copy's lifetime automatically (allocation + deletion); this ensures that the data the
 * handler recieves is always valid.
 *
 * @param[in]  event   event type.
 * @param[in]  sub_event   sub event.
 * @param[in]  value  event value.
 * @param[in]  value  event size of value.
 *
 * @return  the operation status, 0 is OK, others is error.
 */
int32_t qm_event_post(uint16_t event, uint16_t sub_event, void *value, uint32_t size);

/**
 * Post local event from isr
 *
 * @param[in]  event   event type.
 * @param[in]  sub_event   sub event.
 * @param[in]  value  event value.
 * @param[in]  value  event size of value.
 *
 * @return  the operation status, 0 is OK, others is error.
 */
int32_t qm_event_post_from_isr(uint16_t event, uint16_t sub_event, void *value, uint32_t size);

/**
 * @brief Posts an event to the specified event loop. The event loop library keeps a copy of event_data and manages
 * the copy's lifetime automatically (allocation + deletion); this ensures that the data the
 * handler recieves is always valid.
 *
 * @param[in]  event   event type.
 * @param[in]  sub_event   sub event.
 * @param[in]  value  event value.
 * @param[in]  value  event size of value.
 * @param[in]  arg    User defined parameters
 *
 * @return  the operation status, 0 is OK, others is error.
 */
int32_t qm_event_ext_post(uint16_t event, uint16_t sub_event, void *value, uint32_t size, void *arg);

/**
 * Post local event from isr
 *
 * @param[in]  event   event type.
 * @param[in]  sub_event   sub event.
 * @param[in]  value  event value.
 * @param[in]  value  event size of value.
 * @param[in]  arg    User defined parameters
 *
 * @return  the operation status, 0 is OK, others is error.
 */
int32_t qm_event_ext_post_from_isr(uint16_t event, uint16_t sub_event, void *value, uint32_t size, void *arg);


#endif
