#ifndef QM_OTA_H
#define QM_OTA_H

#include "qm.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * init ota partition
 *
 * @note   when ota start, maybe it need init something
 * @param  arg  extra info for ota init
 *
 * @return  0 : On success, -1 : If an error 
 * occurred with any step
 */
int qm_ota_start(void *arg);

/**
 * Write data to an area on ota partition
 *
 * @param  off_set     Point to the start address that the data is written to, and
 *                     point to the last unwritten address after this function is
 *                     returned, so you can call this function serval times without
 *                     update this start address.
 * @param  inbuf       point to the data buffer that will be written to flash
 * @param  in_buf_len  The length of the buffer
 *
 * @return  0 : On success, -1 : If an error occurred with any step
 */
int qm_ota_write(uint32_t *off_set, uint8_t *in_buf, uint32_t in_buf_len);

/**
 * Read data from an area on ota Flash to data buffer in RAM
 *
 * @param  off_set      Point to the start address that the data is read, and
 *                      point to the last unread address after this function is
 *                      returned, so you can call this function serval times without
 *                      update this start address.
 * @param  out_buf      Point to the data buffer that stores the data read from flash
 * @param  out_buf_len  The length of the buffer
 *
 * @return  0 : On success, -1 : If an error occurred with any step
 */
int qm_ota_read(uint32_t *off_set, uint8_t *out_buf, uint32_t out_buf_len);

/**
 * ota end
 *
 * @param  arg  extra info for ota end
 *
 * @return  0 : On success, -1 : If an error occurred with any step
 */
int qm_ota_end(void *arg);



#ifdef __cplusplus
}
#endif

#endif /* QM_OTA_H */