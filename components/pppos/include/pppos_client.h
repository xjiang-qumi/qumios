#ifndef _PPPOS_CLINET_H_
#define _PPPOS_CLINET_H_

#include "qm_config.h" 

#ifndef CONFIG_PPPOS_CLIENT_TASK_SIZE
#define CONFIG_PPPOS_CLIENT_TASK_SIZE    4096
#endif

#ifndef CONFIG_PPPOS_CLIENT_TASK_PRIO
#define CONFIG_PPPOS_CLIENT_TASK_PRIO    20
#endif

/**
 * @brief  PPPoS client link event types.
 */
typedef enum {
    PPPOS_CLIENT_EVENT_LINK_ON,  /**< PPPoS link established. */
    PPPOS_CLIENT_EVENT_LINK_OFF  /**< PPPoS link dropped. */
}pppos_client_event_t;

/**
 * @brief  Initialize the PPPoS client.
 * @return 0 on success, negative value on failure.
 */
int pppos_client_init(void);


#endif
