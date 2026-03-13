

#ifndef _QM_NET_H_
#define _QM_NET_H_

#include "qm_types.h"


/**
 * @brief The structure of network connection(TCP or SSL).
 *   The user has to allocate memory for this structure.
 */

struct util_network;
typedef struct util_network util_network_t, *util_network_pt;

struct util_network {
    const char *pHostAddress;
    uint16_t port;
    
    int ca_crt_len;
    const char *ca_crt;              /*CA certificate*/  /**< NULL, TCP connection; NOT NULL, SSL connection */

    int client_crt_len;
    const char *client_crt;          /*Client certificate*/

    int client_key_len;
    const char *client_key;          /*Client certificate's private key*/

    /**< connection handle: NULL, NOT connection; NOT NULL, handle of the connection */
    void *handle;

    /**< Read data from server function pointer. */
    int (*read)(util_network_pt, char *, uint32_t, uint32_t);

    /**< Send data to server function pointer. */
    int (*write)(util_network_pt, const char *, uint32_t, uint32_t);

    /**< Disconnect the network */
    int (*disconnect)(util_network_pt);

    /**< Establish the network */
    int (*connect)(util_network_pt);
};


/**
 * @brief  Read data from the network connection.
 * @param  pNetwork [IN/OUT] Pointer to the network handle.
 * @param  buffer [OUT] Buffer to store received data.
 * @param  len [IN] Number of bytes to read.
 * @param  timeout_ms [IN] Read timeout in milliseconds.
 * @return Number of bytes read, or negative value on failure.
 */
int util_net_read(util_network_pt pNetwork, char *buffer, uint32_t len, uint32_t timeout_ms);

/**
 * @brief  Write data to the network connection.
 * @param  pNetwork [IN/OUT] Pointer to the network handle.
 * @param  buffer [IN] Data buffer to send.
 * @param  len [IN] Number of bytes to write.
 * @param  timeout_ms [IN] Write timeout in milliseconds.
 * @return Number of bytes written, or negative value on failure.
 */
int util_net_write(util_network_pt pNetwork, const char *buffer, uint32_t len, uint32_t timeout_ms);

/**
 * @brief  Disconnect the network connection.
 * @param  pNetwork [IN/OUT] Pointer to the network handle.
 * @return 0 on success, negative value on failure.
 */
int util_net_disconnect(util_network_pt pNetwork);

/**
 * @brief  Establish a network connection.
 * @param  pNetwork [IN/OUT] Pointer to the network handle.
 * @return 0 on success, negative value on failure.
 */
int util_net_connect(util_network_pt pNetwork);

/**
 * @brief  Initialize network handle with host and port.
 * @param  pNetwork [IN/OUT] Pointer to the network handle.
 * @param  host [IN] Hostname or IP address string.
 * @param  port [IN] Port number.
 * @param  ca_crt [IN] CA certificate for TLS, or NULL for plain TCP.
 * @return 0 on success, negative value on failure.
 */
int util_net_init(util_network_pt pNetwork, const char *host, uint16_t port, const char *ca_crt);

/**
 * @brief  Set client certificate data for mutual TLS authentication.
 * @param  pNetwork [IN/OUT] Pointer to the network handle.
 * @param  client_crt [IN] Client certificate string.
 * @param  len [IN] Length of the client certificate.
 * @return 0 on success, negative value on failure.
 */
int util_net_set_client_cert_data(util_network_pt pNetwork, const char *client_crt, int len);

/**
 * @brief  Set client private key data for mutual TLS authentication.
 * @param  pNetwork [IN/OUT] Pointer to the network handle.
 * @param  client_key [IN] Client private key string.
 * @param  len [IN] Length of the client key.
 * @return 0 on success, negative value on failure.
 */
int util_net_set_client_key_data(util_network_pt pNetwork, const char *client_key, int len);

#endif /* IOT_COMMON_NET_H */
