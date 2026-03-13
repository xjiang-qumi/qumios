#ifndef __DATA_CHANNEL_H__
#define __DATA_CHANNEL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "qm_types.h"

/**
 * @brief  Create a data channel server instance with specified buffer sizes.
 *
 * @param  rd_size [IN] Read buffer size in bytes (server reads from client).
 * @param  wr_size [IN] Write buffer size in bytes (server writes to client).
 *
 * @return Pointer to the server handle on success, NULL on failure.
 */
void *data_channel_server_creat(uint32_t rd_size, uint32_t wr_size);

/**
 * @brief  Create a data channel client instance.
 *
 * @return Pointer to the client handle on success, NULL on failure.
 */
void *data_channel_client_creat(void);

/**
 * @brief  Connect a client to a server, sharing the ring buffers.
 *
 * @param  handle_server [IN] Server handle created by data_channel_server_creat().
 * @param  handle_client [IN/OUT] Client handle created by data_channel_client_creat().
 *
 * @return 0 on success, negative error code on failure.
 */
int data_channel_connect(void *handle_server, void *handle_client);

/**
 * @brief  Write data to a channel (server or client handle).
 *
 * @param  handle [IN] Server or client handle.
 * @param  data [IN] Pointer to the data buffer to write.
 * @param  len [IN] Number of bytes to write.
 * @param  timeout [IN] Timeout in milliseconds to wait for space.
 *
 * @return Number of bytes written on success, negative error code on failure.
 */
int data_channel_write(void *handle, uint8_t *data, uint32_t len, uint32_t timeout);

/**
 * @brief  Read data from a channel (server or client handle).
 *
 * @param  handle [IN] Server or client handle.
 * @param  data [OUT] Pointer to the buffer to store read data.
 * @param  len [IN] Number of bytes to read.
 * @param  timeout [IN] Timeout in milliseconds to wait for data.
 *
 * @return Number of bytes read on success, negative error code on failure.
 */
int data_channel_read(void *handle, uint8_t *data, uint32_t len, uint32_t timeout);

/**
 * @brief  Destroy a channel handle and release all associated resources.
 *
 * @param  handle [IN] Server or client handle to destroy.
 *
 * @return 0 on success, negative error code on failure.
 */
int data_channel_destroy(void *handle);

#ifdef __cplusplus
}
#endif


#endif /* __DATA_CHANNEL_H__ */