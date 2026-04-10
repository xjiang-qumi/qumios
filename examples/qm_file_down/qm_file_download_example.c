#include "qm.h"
#include "qm_spec_api.h"
#include "qm_iot_core.h"
#include "qm_work.h"
#include "qm_iot_config.h"
#include "qm_time.h"
#include "qm_string.h"
#include "util_httpc.h"

#define LOG_TAG "EXAMPLE_FILE_DOWNLOAD"
#define FILE_BUF_SIZE    (1024*8)
#define RECV_TIMEOUT     5000
#define FILE_URL_LEN     256

/**
 * @brief 文件下载请求信息结构
 *
 * 目前仅包含要下载的 URL 地址。
 */
typedef struct {
    uint8_t url[FILE_URL_LEN];
} file_download_info_t;


static file_download_info_t g_file_download_info = {0};
static qm_task_t task;
static qm_queue_t download_queue;

/**
 * @brief 从物模型中提取 URL 并加入下载队列。
 *
 * 该函数将从输入属性中读取字符串 URL，检查长度是否合法，
 * 然后发送到下载线程处理队列。
 */
void file_set_url(qm_spec_property_t *property)
{
    int len = 0;
    if(property == NULL) {
        return ;
    }
    qm_spec_property_unpack_string(property, NULL, &len);
    if(len == 0 || len >= sizeof(g_file_download_info.url)){
        return ;
    }
    qm_spec_property_unpack_string(property, g_file_download_info.url, &len);
    QM_LOGD(LOG_TAG, "url:%s", g_file_download_info.url);
    qm_queue_send(&download_queue, &g_file_download_info, sizeof(file_download_info_t));
    memset(&g_file_download_info, 0, sizeof(file_download_info_t));
}

/**
 * @brief 测试用的 URL 设置函数,网络连接成功后调用
 *
 * 该函数将一个固定的 raw 文件下载地址发送到下载队列，
 * 便于本地调试和验证下载功能。
 */
void test_file_set_url(void){
    strcpy(g_file_download_info.url, "https://gitee.com/ycom-yf/resource/raw/main/demo.mp3");
    qm_queue_send(&download_queue, &g_file_download_info, sizeof(file_download_info_t));
}

/**
 * @brief 文件下载线程入口。
 *
 * 该线程从队列中获取下载请求，然后调用 HTTP 客户端执行 GET 请求，
 * 并将接收到的数据块交给后续处理。
 */
static void download_thread(void *args)
{
    int ret = QM_EOK;
    unsigned int size = 0;
    char *file_buf = NULL;
    file_download_info_t download_info;
    httpclient_t http = {0};              
    httpclient_data_t http_data = {0};
    char ca_cert[32] = "";
    uint32_t ca_cert_len = 0;

    file_buf = (char *)qm_malloc(FILE_BUF_SIZE);
    if(file_buf == NULL){
        return -QM_ENOMEM;
    }
    while (1)
    {
        memset(&download_info, 0, sizeof(file_download_info_t));
        qm_queue_recv(&download_queue, &download_info, &size, QM_WAIT_FOREVER);

        memset(&http, 0, sizeof(httpclient_t));
        memset(&http_data, 0, sizeof(httpclient_data_t));
        memset(file_buf, 0, FILE_BUF_SIZE);
        http_data.response_buf = file_buf;
        http_data.response_buf_len = FILE_BUF_SIZE;

        /* 发起 HTTP 请求，下载文件内容 */
        QM_LOGD(LOG_TAG, "url: %s", download_info.url);
        do {
            ret = http_client_common(&http, download_info.url, 443, ca_cert, HTTPCLIENT_GET, RECV_TIMEOUT, &http_data);
            if(ret == QM_EOK && http.response_code == 200){
                // TODO: 接收到了下载的文件数据，推送到文件处理队列
            } else {
                QM_LOGD(LOG_TAG, "http_client_common failed, ret:%d, response_code:%d", ret, http.response_code);
                break;
            }

        } while (http_data.is_more);

        QM_LOGD(LOG_TAG, "content_len:%d", http_data.response_content_len);
        http_client_close(&http);
    }
}

/**
 * @brief 初始化文件下载模块。
 *
 * 创建下载请求队列，并启动下载线程。
 */
void qm_file_download_init(void)
{
    qm_queue_new(&download_queue, 1, sizeof(file_download_info_t));
    qm_task_new_to_core(&task, "download_example", download_thread, (void *)NULL, 4096, 2, 1);
}
