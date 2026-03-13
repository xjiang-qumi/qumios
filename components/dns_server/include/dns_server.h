
#ifndef _DNS_SERVER_H_
#define _DNS_SERVER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "qm_config.h"

#if CONFIG_DNS_SERVER_SUPPORT

/**
 * @brief  Register a domain-to-IP mapping with the DNS server.
 * @param  domain [IN] Null-terminated domain name string.
 * @param  ip [IN] Null-terminated IP address string.
 * @return 0 on success, negative value on failure.
 */
int dns_server_ip_register(const char *domain, const char *ip);

/**
 * @brief  Start the DNS server.
 * @return 0 on success, negative value on failure.
 */
int dns_server_start(void);

/**
 * @brief  Stop the DNS server.
 * @return 0 on success, negative value on failure.
 */
int dns_server_stop(void);

#endif


#ifdef __cplusplus
}
#endif

#endif /* DNS_SERVER_H */

