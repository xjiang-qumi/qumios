#include "qm.h"
#include "qm_tls.h"

#include "esp_idf_version.h"

#define LOG_TAG "TLS"

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)

#include "esp_tls.h"
#include "sys/time.h"
#include "sys/socket.h"
#include "sys/queue.h"
#include <unistd.h>

#define QM_TLS_ESTABLISH_TIMEOUT    (60 * 1000)

typedef struct 
{
    esp_tls_t      *tls;
    esp_tls_cfg_t   cfg;
    int             sockfd;
}qm_tls_hal_handle_t;

static struct timeval* qm_tls_hal_utils_ms_to_timeval(int timeout_ms, struct timeval *tv)
{
    if (timeout_ms == -1) {
        return NULL;
    }
    tv->tv_sec = timeout_ms / 1000;
    tv->tv_usec = (timeout_ms - (tv->tv_sec * 1000)) * 1000;
    return tv;
}

void *qm_tls_establish(const char *host, uint16_t port, const qm_tls_cfg_t *cfg)
{
    if(host == NULL || cfg == NULL){
        return NULL;
    }

    qm_tls_hal_handle_t *tls_handle = (qm_tls_hal_handle_t *)qm_malloc(sizeof(qm_tls_hal_handle_t));
    if(tls_handle == NULL){
        return NULL;
    }
    memset(tls_handle, 0, sizeof(qm_tls_hal_handle_t));

    tls_handle->cfg.skip_common_name = true;

    QM_LOGD(LOG_TAG, "[mqtt_host]:%s", host);
    QM_LOGD(LOG_TAG, "[post]:%d", port);

    if(cfg->ca_crt && cfg->ca_crt_len){
        tls_handle->cfg.cacert_pem_buf = (void *)cfg->ca_crt;
        tls_handle->cfg.cacert_pem_bytes = cfg->ca_crt_len + 1;
    }

    if(cfg->client_crt && cfg->client_crt_len){
        tls_handle->cfg.clientcert_pem_buf = (void *)cfg->client_crt;
        tls_handle->cfg.clientcert_pem_bytes = cfg->client_crt_len + 1;
    }

    if(cfg->client_key && cfg->client_key_len){
        tls_handle->cfg.clientkey_pem_buf = (void *)cfg->client_key;
        tls_handle->cfg.clientkey_pem_bytes = cfg->client_key_len + 1;       
    }

    tls_handle->cfg.timeout_ms = QM_TLS_ESTABLISH_TIMEOUT;
    
    tls_handle->tls = esp_tls_init();
    if (tls_handle->tls  == NULL) {
        QM_LOGE(LOG_TAG, "Failed to initialize new connection object");
        goto exit_failure;
    }

    if (esp_tls_conn_new_sync(host, strlen(host), port, &tls_handle->cfg, tls_handle->tls) <= 0) {
        QM_LOGE(LOG_TAG, "Failed to open a new connection");
        goto exit_failure;
    }

    if (esp_tls_get_conn_sockfd(tls_handle->tls, &tls_handle->sockfd) != ESP_OK) {
        QM_LOGE(LOG_TAG, "Error in obtaining socket fd for the session");
        goto exit_failure;
    }
    QM_LOGD(LOG_TAG, "connection COMPLETE");

    return (void *)tls_handle;

exit_failure:

    if(tls_handle->tls){
        esp_tls_conn_destroy(tls_handle->tls);
        tls_handle->tls = NULL;
    }
    qm_free(tls_handle);
    return NULL;
}

int32_t qm_tls_write(void *handle, const char *buf, int len, uint32_t timeout_ms)
{
    struct timeval t;
    fd_set errset;
    fd_set writeset;
    int length = 0, ret = 0;
    qm_tls_hal_handle_t *tls_handle = (qm_tls_hal_handle_t *)handle;
    
    if (NULL == handle){
        QM_LOGE(LOG_TAG, "handle is NULL");
        return -QM_EINVAL;
    }

    FD_ZERO(&writeset);
    FD_ZERO(&errset);
    FD_SET(tls_handle->sockfd, &writeset);
    FD_SET(tls_handle->sockfd, &errset);
    ret = select(tls_handle->sockfd + 1, NULL, &writeset, &errset, qm_tls_hal_utils_ms_to_timeval(timeout_ms, &t));
    if (ret > 0 && FD_ISSET(tls_handle->sockfd, &errset)) {
        int sock_errno = 0;
        uint32_t optlen = sizeof(sock_errno);
        getsockopt(tls_handle->sockfd, SOL_SOCKET, SO_ERROR, &sock_errno, &optlen);
        QM_LOGE(LOG_TAG, "poll_write select error %d, errno = %s, fd = %d", sock_errno, strerror(sock_errno), tls_handle->sockfd);
        return -QM_ERROR;
    }else if (ret == 0) {
        QM_LOGE(LOG_TAG, "poll_write: select - Timeout before any socket was ready!");
        return -QM_ETIMEOUT;
    }

    length = esp_tls_conn_write(tls_handle->tls, (const unsigned char *) buf, len);
    if (length < 0) {
        QM_LOGE(LOG_TAG,  "esp_tls_conn_write error, errno=%s", strerror(errno));
    }
    
    return (length > 0) ? length : -QM_ERROR;
}

int32_t qm_tls_read(void * handle, char *buf, int len, uint32_t timeout_ms)
{
    int ret = 0;
    int rxLen = 0;
    int remain = 0;
    fd_set readset;
    fd_set errset;
    struct timeval t;
    qm_tls_hal_handle_t *tls_handle = (qm_tls_hal_handle_t *)handle;
    
    if (NULL == handle){
        QM_LOGE(LOG_TAG, "handle is NULL");
        return -QM_EINVAL;
    }

    FD_ZERO(&readset);
    FD_ZERO(&errset);
    FD_SET(tls_handle->sockfd, &readset);
    FD_SET(tls_handle->sockfd, &errset);

    if (tls_handle->tls && (remain = esp_tls_get_bytes_avail(tls_handle->tls) > 0)) {
        goto __next;
    }

    ret = select(tls_handle->sockfd + 1, &readset, NULL, &errset, qm_tls_hal_utils_ms_to_timeval(timeout_ms, &t));
    if (ret > 0 && FD_ISSET(tls_handle->sockfd, &errset)) {
        int sock_errno = 0;
        uint32_t optlen = sizeof(sock_errno);
        getsockopt(tls_handle->sockfd, SOL_SOCKET, SO_ERROR, &sock_errno, &optlen);
        QM_LOGE(LOG_TAG, "poll_read select error %d, errno = %s, fd = %d", sock_errno, strerror(sock_errno), tls_handle->sockfd);
       return -QM_EIO;
    } else if (ret == 0) {
        return 0;
    }

__next:
    rxLen = esp_tls_conn_read(tls_handle->tls, (unsigned char *)buf, len);
    if (rxLen < 0) {
        QM_LOGE(LOG_TAG, "esp_tls_conn_read error, errno=%s", strerror(errno));
        return -QM_EIO;
    } 
    return rxLen;
}

int32_t qm_tls_destroy(void *handle)
{
    qm_tls_hal_handle_t *tls_handle = (qm_tls_hal_handle_t *)handle;
    if (NULL == handle){
        QM_LOGE(LOG_TAG, "handle is NULL");
        return -QM_EINVAL;
    }

    if(tls_handle->tls){
        esp_tls_conn_destroy(tls_handle->tls);
        tls_handle->tls = NULL;
    }
    qm_free(tls_handle);
    return QM_EOK;
}

#else
#include "openssl/ssl.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/err.h"
#include "esp_system.h"

#define LOG_TAG "hal tls"

typedef struct _TLSDataParams
{
    int server_fd;
    SSL_CTX *ctx;
    SSL *ssl;
} TLSDataParams_t, *TLSDataParams_pt;

#define SSL_READ_BUFFER_LEN 8120

void *qm_tls_establish(const char *host, uint16_t port, const char *ca_crt, uint32_t ca_crt_len)
{
    TLSDataParams_pt pTlsData = NULL;
    SSL_CTX *ctx = NULL;
    SSL *ssl = NULL;
    int ret = -1;
    int server_fd = -1;

    struct sockaddr_in addr;
    struct hostent *hostent_content = NULL;

    QM_LOGD(LOG_TAG, "host name: %s", host);
    hostent_content = gethostbyname(host);
    if (hostent_content == NULL)
    {
        QM_LOGE(LOG_TAG, "gethostbyname err");
        return NULL;
    }

    server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_fd < 0)
    {
        QM_LOGE(LOG_TAG, "socket creat err");
        return NULL;
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = *(uint32_t *)(*hostent_content->h_addr_list);
    addr.sin_port = htons(port);

    QM_LOGD(LOG_TAG, "ip %s port %d fd %d\n", inet_ntoa(addr.sin_addr), port, server_fd);

    pTlsData = malloc(sizeof(TLSDataParams_t));

    if (NULL == pTlsData)
    {
        return NULL;
    }

    ret = connect(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret)
    {
        QM_LOGE(LOG_TAG, "connect failed");
        goto err_exit;
    }

    QM_LOGD(LOG_TAG, "server_fd %d", server_fd);

    ctx = SSL_CTX_new(TLSv1_1_client_method());
    if (!ctx)
    {
        QM_LOGE(LOG_TAG, "SSL_CTX_new, ret: %d, free_heap :%u", (uint32_t)ctx, esp_get_free_heap_size());
        goto err_exit;
    }
    QM_LOGD(LOG_TAG, "set SSL context read buffer size free_heap: %d", esp_get_free_heap_size());
    SSL_CTX_set_default_read_buffer_len(ctx, SSL_READ_BUFFER_LEN);
    ssl = SSL_new(ctx);
    if (!ssl)
    {
        QM_LOGE(LOG_TAG, "SSL_new failed free_heap :%u", esp_get_free_heap_size());
        goto err_exit;
    }

    SSL_set_fd(ssl, server_fd);

    pTlsData->server_fd = server_fd;
    pTlsData->ctx = ctx;
    pTlsData->ssl = ssl;

    ret = SSL_connect(ssl);
    if (!ret)
    {
        QM_LOGE(LOG_TAG, "SSL_connect, ret: %d,free_heap :%u", ret, esp_get_free_heap_size());
        goto err_exit;
    }

    QM_LOGD(LOG_TAG, "ssl connect success");

    QM_LOGD(LOG_TAG, "SSL_new, ret: %d, free_heap :%u", (uint32_t)ssl, esp_get_free_heap_size());

    return (void*)pTlsData;

err_exit:

    if (pTlsData){
        free(pTlsData);
        pTlsData = NULL;
    }

    if (ctx){
        SSL_CTX_free(ctx);
        ctx = NULL;
    }
    if (ssl){
        SSL_free(ssl);
        ssl = NULL;
    }

    if (server_fd >= 0){
        close(server_fd);
    }

    QM_LOGD(LOG_TAG, "platform_ssl_connect end free_heap :%u", esp_get_free_heap_size());
    return NULL;
}

int32_t qm_tls_destroy(void *handle)
{
    TLSDataParams_pt pTlsData = NULL;
    if (NULL == handle){
        QM_LOGE(LOG_TAG, "handle is NULL");
        return -QM_EINVAL;
    }
    pTlsData = (TLSDataParams_t *)handle;

    close(pTlsData->server_fd);

    if (pTlsData->ssl){
        SSL_free(pTlsData->ssl);
    }
    if (pTlsData->ctx){
        SSL_CTX_free(pTlsData->ctx);
    }
    if (pTlsData){
        free(pTlsData);
    }

    QM_LOGD(LOG_TAG, "ssl_disconnect");
    return QM_EOK;
}

int32_t qm_tls_write(void *handle, const char *buf, int len, uint32_t timeout_ms)
{
    TLSDataParams_t *pTlsData = (TLSDataParams_t *)handle;
    int length = 0, ret = 0;
    if (NULL == handle){
        QM_LOGE(LOG_TAG, "handle is NULL");
        return -QM_EINVAL;
    }

    while (length < len)
    {
        ret = SSL_write(pTlsData->ssl, (char *)(buf + length), (len - length));
        QM_LOGD(LOG_TAG, "ssl send =%d", ret);
        if (ret < 0){
            length = ret;
            break;
        }
        else if (ret > 0){
            length += ret;
        }
        else{
            return length;
        }
    }

    return (length > 0) ? length : -1;
}

int32_t qm_tls_read(void * handle, char *buf, int len, uint32_t timeout_ms)
{
    TLSDataParams_t *pTlsData = (TLSDataParams_t *)handle;
    int rxLen = 0;
    int ret = 0;
    int fd = pTlsData->server_fd;
    fd_set readfds;
    struct timeval t;

    if (NULL == handle){
        QM_LOGE(LOG_TAG, "handle is NULL");
        return -QM_EINVAL;
    }

    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);

    t.tv_sec = timeout_ms / 1000;
    t.tv_usec = (timeout_ms % 1000) * 1000;

    while (len > 0)
    {
        if (SSL_pending(pTlsData->ssl)){
            ret = SSL_read(pTlsData->ssl, buf, len);
        }
        else{

            if (timeout_ms == 0){
                break;
            }
            //platform_log("timeout_ms %u, cur %u", timeout_ms, HAL_UptimeMs());

            ret = select(fd + 1, &readfds, NULL, NULL, &t);
            if (ret > 0)
            {
                if (FD_ISSET(fd, &readfds))
                {
                    ret = SSL_read(pTlsData->ssl, buf, len);
                    if (ret < 0)
                    {
                        QM_LOGE(LOG_TAG, "select ret %d", ret);
                    }
                }
            }
            else if (ret == 0)
            {
                break;
            }
        }

        if (ret >= 0)
        {
            rxLen += ret;
            buf += ret;
            len -= ret;
        }

        if (ret < 0)
        {
            QM_LOGE(LOG_TAG, "socket read err");
            return -QM_EIO;
        }

        if (len == 0)
        {
            QM_LOGD(LOG_TAG, "recv len %d", rxLen);
            return rxLen;
        }
    }

    return rxLen;
}
#endif