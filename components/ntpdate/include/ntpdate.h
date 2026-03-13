#ifndef __NTPDATE_H__
#define __NTPDATE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "qm_config.h"
#include "qm_types.h"

#if CONFIG_NTPDATE_SUPPORT

#ifndef CONFIG_NTPDATE_TASK_SIZE  
#define CONFIG_NTPDATE_TASK_SIZE                 4*1024    
#endif

#ifndef CONFIG_NTPDATE_TASK_PRIO  
#define CONFIG_NTPDATE_TASK_PRIO                 21    
#endif

#ifndef CONFIG_NTPDATE_PERIODIC_SYNC_SUPPORT  
#define CONFIG_NTPDATE_PERIODIC_SYNC_SUPPORT              1    
#endif

#if CONFIG_NTPDATE_PERIODIC_SYNC_SUPPORT

#ifndef CONFIG_NTPDATE_PERIODIC_SYNC_INTERVAL 
#define CONFIG_NTPDATE_PERIODIC_SYNC_INTERVAL           12*3600   
#endif

#endif

/**
 * @brief  NTP synchronization result.
 */
typedef enum {
    NTPDATE_RES_SUCCESS, /**< Time synchronization succeeded. */
    NTPDATE_RES_FAIL,    /**< Time synchronization failed. */
}ntpdate_res_t;

/**
 * @brief  Callback invoked on NTP synchronization completion.
 */
typedef void (*ntpdate_complete_cb)(ntpdate_res_t res);

/**
 * @brief  Start NTP time synchronization.
 * @param  cb [IN] Callback function invoked when synchronization completes.
 * @return 0 on success, negative value on failure.
 */
int ntpdate_start(ntpdate_complete_cb cb);


#endif

#ifdef __cplusplus
}
#endif

#endif