#include "qm.h"
#include "qm_ota.h"
#include "qm_work.h"
#include "qm_time.h"

#include "json_parser.h"
#include "qm_iot_ota.h"
#include "qm_iot_mqtt.h"
#include "qm_iot_config.h"
#include "qm_iot_common.h"
#include "qm_iot_crypto.h"
#include "qm_iot_streams.h"
#include "qm_iot_jobs.h"
#include "qm_iot_jobs_profile.h"

#include "qm_utils_base64.h"
#include "qm_utils_string.h"

#define LOG_TAG "ota"

#define NUM_OF_BLOCKS_REQUESTED     ( 1U )

#ifndef CONFIG_QM_IOT_OTA_RETRY_COUNT
#define CONFIG_QM_IOT_OTA_RETRY_COUNT      (10)
#endif

#ifndef CONFIG_QM_IOT_OTA_TIMOUT_MS
#define CONFIG_QM_IOT_OTA_TIMOUT_MS        (5 * 1000)
#endif

#define TIMESTAMP_2000  943891200  /* 1970~2000 in seconds */

#define QM_IOT_OTA_PROGRESS_STR_NUM (10)

#define QM_IOT_OTA_UPDATA_COMPLETE  (100)

#define TOPIC_MAX_STR_LEN   (128)

#define DECODE_MAX_STR_LEN   (384)

#define QM_IOT_OTA_TYPE             (QM_IOT_STREAMS_DATA_TYPE_CBOR)

#define QM_IOT_OTA_VER_RETRY_TIMEOUT    (10 * 1000)

#define OTA_VERSION_INFORM_TOPIC            ("/%u/%s/device/version") // /${pid}/${did}/device/otaInform
#define OTA_VERSION_INFORM_REPLAY_TOPIC     ("/%u/%s/device/versionReply") // /${pid}/${did}/device/otaInformReply

#define OTA_START_TOPIC                     ("/%u/%s/device/startOta") // /${pid}/${did}/device/startOta
#define OTA_START_REPLY_TOPIC               ("/%u/%s/device/startOtaReply") // /${pid}/${did}/device/startOtaReply

#define OTA_NOTIFY_TOPIC                    ("/%u/%s/device/notifyOta") // /${pid}/${did}/device/notifyOta
#define OTA_NOTIFY_REPLY_TOPIC              ("/%u/%s/device/notifyOtaReply") // /${pid}/${did}/device/notifyOtaReply

#define OTA_PROGRESS_TOPIC                  ("/%u/%s/device/otaProgress") // /${pid}/${did}/device/notifyOtaReply

#define OTA_START_PAYLOAD  ("{\"method\":\"startOta\",\"id\":%u,\"timestamp\":%u}")  
#define OTA_VERSION_INFORM_PAYLOAD  ("{\"method\":\"version\",\"id\":%u,\"params\":{\"sdkVersion\":\"%s\",\"moduleVersion\":%u,\"silentOTA\":%u},\"timestamp\":%u}")  
#define OTA_NOTIFY_REPLY_PAYLOAD  ("{\"method\":\"notifyOtaReply\",\"id\":%u,\"code\":0,\"params\":{\"devState\":%u},\"timestamp\":%u}")  
#define OTA_PROGRESS_NOTIFY_PAYLOAD  ("{\"method\":\"otaProgress\",\"id\":%u,\"code\":0,\"params\":%s,\"timestamp\":%u}")  


#define ID_KEY                      "id"
#define CODE_KEY                    "code"
#define TYPE_KEY                    "type"
#define VESION_KEY                  "version"
#define PARAMS_KEY                  "params"
#define OTA_STATE_KEY               "otaState"
#define START_TIME_KEY              "startTime"
#define END_TIME_KEY                "endTime"

typedef struct 
{
    uint32_t msg_id;
    int lastReceivedblockId;
    uint32_t pid;
    char did[CONFIG_QM_IOT_DID_MAX_LEN + 1];
    void *mqtt_handle;
    void *crypto_handle;
    char jobs_id[TOPIC_MAX_STR_LEN + 1];
    int progress_details;
    uint8_t currentFileId;
    uint32_t currentBlockOffset;
    uint32_t totalBytesReceived;
    qm_iot_jobs_file_t documentFields;    
    int streams_sub_id;
    int32_t job_update_pub_notify;
    qm_iot_steams_context_t streams_context;
    void *user_data;
    qm_iot_ota_event_handler_t event_handler;
    char *pucSignature;
    uint8_t signature[DECODE_MAX_STR_LEN];
    uint32_t signatureLen;
    int retry_count;
    qm_work_t  retry_work;
    qm_work_t  ota_notify_work;
    qm_work_t  ota_success_work;
    qm_mutex_t ota_lock;  // 添加互斥锁
#if CONFIG_QM_IOT_SLIENT_OTA_SUPPORT
    uint32_t next_rand_time;
#endif
}qm_iot_ota_handle_t;

typedef struct 
{
    int packet_id;
    char topic[TOPIC_MAX_STR_LEN + 1];
}qm_iot_ota_topic_map_t;

static qm_iot_ota_handle_t *g_ota_handle = NULL;
static qm_mutex_t g_ota_global_lock;

static void qm_iot_ota_start_aciton(void *arg);
static void qm_iot_event_handler(qm_iot_ota_handle_t *ota_handler, qm_iot_ota_event_t *event);

static void qm_iot_ota_retry_timeout(void *arg)
{
    int decodedDataLength;
    qm_iot_ota_event_t event = {0};
    uint8_t decodedData[QM_IOT_STREAMS_REQUEST_BUFFER_SIZE] = {0};
    qm_iot_ota_handle_t *ota_handler = (qm_iot_ota_handle_t *)arg;
    if(ota_handler == NULL){
        return;
    }

    if(ota_handler->retry_count++ >= CONFIG_QM_IOT_OTA_RETRY_COUNT){
        event.type = QM_IOT_OTA_EVENT_FAILED;
        qm_iot_event_handler(ota_handler, &event);
        return;
    }

    if(ota_handler->totalBytesReceived < ota_handler->documentFields.fileSize){

        decodedDataLength = qm_iot_steams_create_request(QM_IOT_OTA_TYPE, 
                                                        ota_handler->currentFileId, CONFIG_QM_IOT_STREAMS_BLOCK_SIZE,
                                                        ota_handler->currentBlockOffset, NUM_OF_BLOCKS_REQUESTED, 
                                                        (char *)decodedData, QM_IOT_STREAMS_REQUEST_BUFFER_SIZE);

        qm_iot_mqtt_pub(ota_handler->mqtt_handle, 
                        ota_handler->streams_context.topicGetStream, 
                        decodedData, decodedDataLength, QM_MQTT_QoS0);
    }

    qm_post_delayed_action(&ota_handler->retry_work, qm_iot_ota_retry_timeout, ota_handler, CONFIG_QM_IOT_OTA_TIMOUT_MS);
}

static void qm_iot_ota_success_action(void *arg)
{
    qm_iot_ota_event_t event = {0};
    qm_iot_ota_handle_t *ota_handler = (qm_iot_ota_handle_t *)arg;

    if(ota_handler && ota_handler->job_update_pub_notify){
        ota_handler->job_update_pub_notify = QM_FALSE;
        event.type = QM_IOT_OTA_EVENT_SUCCESS;
        qm_iot_event_handler(ota_handler, &event);
    }
}

static void qm_iot_event_handler(qm_iot_ota_handle_t *ota_handler, qm_iot_ota_event_t *event)
{
    static uint32_t off_set = 0;
    int progress_details = 0;
    int ret = QM_EOK;
    int request_len = 0;
    uint32_t timestamp = 0;
    uint32_t next_timestamp = 0;
    char out_topic[TOPIC_MAX_STR_LEN + 1] = {0};
    char str_progress[QM_IOT_OTA_PROGRESS_STR_NUM + 1] = {0};
    char client_token[32 + 1] = {0};
    char request_data[QM_IOT_STREAMS_REQUEST_BUFFER_SIZE] = {0};
    char progress_data[QM_IOT_STREAMS_PROGRESS_BUFFER_SIZE] = {0};
    qm_iot_jobs_status_t status = QM_IOT_JOBS_INPROGRESS;

    static qm_iot_ota_event_type_t  event_type = QM_IOT_OTA_EVENT_NONE;

    if(ota_handler == NULL ){
        return ;
    }

    switch (event->type)
    {

        case QM_IOT_OTA_EVENT_CANCEL:

            if(event_type < QM_IOT_OTA_EVENT_START || event_type > QM_IOT_OTA_EVENT_PROGRESS_UPADTE){
                break;
            }
            
            qm_cancel_delayed_action(&ota_handler->retry_work);

        #if CONFIG_QM_IOT_SLIENT_OTA_SUPPORT
            qm_srandom(qm_now_ms());
            timestamp = qm_time(NULL);
            if(ota_handler->next_rand_time > timestamp){
                next_timestamp = (ota_handler->next_rand_time - timestamp) * 1000;
            }

            QM_LOGD(LOG_TAG,"next timestamp[%u], now timestamp[%u] except timer[%u]!!", ota_handler->next_rand_time, timestamp, next_timestamp);
            qm_post_delayed_action(&g_ota_handle->ota_notify_work, qm_iot_ota_start_aciton, NULL, next_timestamp);
        #endif

            if(ota_handler->event_handler){
                ota_handler->event_handler(ota_handler, event, ota_handler->user_data);
            }

        break;

        case QM_IOT_OTA_EVENT_READY:
            if(ota_handler->event_handler){
                ota_handler->event_handler(ota_handler, event, ota_handler->user_data);
            }
        break;

        case QM_IOT_OTA_EVENT_START:

            off_set = 0;
            ota_handler->lastReceivedblockId = -1;
            ota_handler->job_update_pub_notify = QM_FALSE;

            qm_ota_start(NULL);

            qm_cancel_delayed_action(&ota_handler->retry_work);
        #if CONFIG_QM_IOT_SLIENT_OTA_SUPPORT
            qm_cancel_delayed_action(&ota_handler->ota_notify_work);    //防止多次调用
        #endif

            qm_iot_crypto_signature_verification_start(ota_handler->crypto_handle,
                                                        ota_handler->documentFields.hash_crypto,
                                                        ota_handler->documentFields.asymmetric_crypto);

            request_len = qm_iot_steams_create_request(QM_IOT_OTA_TYPE, 
                                                        ota_handler->currentFileId, CONFIG_QM_IOT_STREAMS_BLOCK_SIZE,
                                                        ota_handler->currentBlockOffset, NUM_OF_BLOCKS_REQUESTED, 
                                                        request_data, QM_IOT_STREAMS_REQUEST_BUFFER_SIZE);
            QM_LOGD(LOG_TAG, "pub[%s]:%s", event->data.start.topic_getstream, request_data);

            qm_iot_mqtt_pub(ota_handler->mqtt_handle, 
                            event->data.start.topic_getstream, 
                            (uint8_t *)request_data, request_len, QM_MQTT_QoS0);
            
            ota_handler->retry_count = 0;
            qm_post_delayed_action(&ota_handler->retry_work, qm_iot_ota_retry_timeout, ota_handler, CONFIG_QM_IOT_OTA_TIMOUT_MS);
  
            if(ota_handler->event_handler){
                ota_handler->event_handler(ota_handler, event, ota_handler->user_data);
            }
        break;

        case QM_IOT_OTA_EVENT_DATA_INSTALLING:

            qm_cancel_delayed_action(&ota_handler->retry_work);
        #if CONFIG_QM_IOT_SLIENT_OTA_SUPPORT
            qm_cancel_delayed_action(&ota_handler->ota_notify_work);//防止多次调用
        #endif
            qm_ota_write(&off_set, event->data.data_write.ota_data, event->data.data_write.data_len);
            qm_iot_crypto_signature_verification_update(ota_handler->crypto_handle, 
                                                        event->data.data_write.ota_data, event->data.data_write.data_len);
            ota_handler->retry_count = 0;
            qm_post_delayed_action(&ota_handler->retry_work, qm_iot_ota_retry_timeout, ota_handler, CONFIG_QM_IOT_OTA_TIMOUT_MS);
  
        
            if(ota_handler->event_handler){
                ota_handler->event_handler(ota_handler, event, ota_handler->user_data);
            }
        break;
        
        case QM_IOT_OTA_EVENT_PROGRESS_UPADTE:

            qm_cancel_delayed_action(&ota_handler->retry_work);
        #if CONFIG_QM_IOT_SLIENT_OTA_SUPPORT
            qm_cancel_delayed_action(&ota_handler->ota_notify_work);//防止多次调用
        #endif
            if(event->data.progress_update.progress_details == ota_handler->progress_details){
                break;
            }

            if(event->data.progress_update.progress_details != QM_IOT_OTA_UPDATA_COMPLETE){
                status = QM_IOT_JOBS_INPROGRESS;
            }else{
                ret = qm_iot_crypto_verify_finish(ota_handler->crypto_handle,
                                                    ota_handler->signature, 
                                                    ota_handler->signatureLen,
                                                    ota_handler->pucSignature, 
                                                    strlen(ota_handler->pucSignature));
                if(ret != QM_EOK){
                    status = QM_IOT_JOBS_FAILED;
                    event->type = QM_IOT_OTA_EVENT_FAILED;
                    qm_iot_event_handler(ota_handler, event);
                    QM_LOGE(LOG_TAG, "signature_verification failed:%d!!", ret);
                    return ;
                }
                status = QM_IOT_JOBS_SUCCESSED;

            }
            
            ota_handler->retry_count = 0;
            
            
            if( (event->data.progress_update.progress_details / CONFIG_QM_IOT_OTA_PROGRESS_GRADE) && 
                !(event->data.progress_update.progress_details % CONFIG_QM_IOT_OTA_PROGRESS_GRADE))
            {
                qm_snprintf(client_token, 32, "%u", qm_time(NULL));
                qm_snprintf(str_progress, QM_IOT_OTA_PROGRESS_STR_NUM, "%d%%", event->data.progress_update.progress_details);     
                qm_iot_jobs_update(ota_handler->did, strlen(ota_handler->did), ota_handler->jobs_id, strlen(ota_handler->jobs_id),
                                    out_topic, TOPIC_MAX_STR_LEN, NULL);

                request_len = qm_iot_jobs_update_msg(status, str_progress, strlen(str_progress),
                                                    NULL, 0,
                                                    client_token, strlen(client_token),
                                                    request_data, QM_IOT_STREAMS_REQUEST_BUFFER_SIZE);
                if(request_len <= 0){
                    break;
                }
                
                QM_LOGD(LOG_TAG, "pub[%s]:%s", out_topic, request_data);
                
                ota_handler->progress_details = event->data.progress_update.progress_details;
                qm_iot_mqtt_pub(ota_handler->mqtt_handle, out_topic, (uint8_t *)request_data, request_len, QM_MQTT_QoS0);
                
                memset(out_topic, 0, TOPIC_MAX_STR_LEN + 1);
                qm_snprintf(out_topic, TOPIC_MAX_STR_LEN, OTA_PROGRESS_TOPIC, ota_handler->pid, ota_handler->did);
                
                ota_handler->msg_id++;
                qm_snprintf(progress_data, QM_IOT_STREAMS_PROGRESS_BUFFER_SIZE, 
                            OTA_PROGRESS_NOTIFY_PAYLOAD, ota_handler->msg_id, request_data, qm_time(NULL));

                QM_LOGD(LOG_TAG, "pub[%s]:%s", out_topic, progress_data);
                qm_iot_mqtt_pub(ota_handler->mqtt_handle, out_topic, (uint8_t *)progress_data, strlen(progress_data), QM_MQTT_QoS0);
            }

            if(status == QM_IOT_JOBS_SUCCESSED){
                qm_cancel_delayed_action(&ota_handler->ota_success_work);//防止多次调用
                ota_handler->job_update_pub_notify = QM_TRUE;
                qm_post_delayed_action(&ota_handler->ota_success_work, qm_iot_ota_success_action, ota_handler, 1000);
            }

            if(ota_handler->event_handler){
                ota_handler->event_handler(ota_handler, event, ota_handler->user_data);
            }
        break;

        case QM_IOT_OTA_EVENT_SUCCESS:
            QM_LOGD(LOG_TAG, "qm_ota complete");

            if(ota_handler->event_handler){
                ota_handler->event_handler(ota_handler, event, ota_handler->user_data);
            }
            
            qm_ota_end(NULL);    
            ota_handler->job_update_pub_notify = QM_FALSE; 
        break;

        case QM_IOT_OTA_EVENT_FAILED:
            QM_LOGD(LOG_TAG, "qm_ota failed!!");

            ota_handler->job_update_pub_notify = QM_FALSE;
            qm_iot_mqtt_unsub(ota_handler->mqtt_handle, ota_handler->streams_context.topicStreamData);

        #if CONFIG_QM_IOT_SLIENT_OTA_SUPPORT
            qm_srandom(qm_now_ms());
            timestamp = qm_time(NULL);
            if(ota_handler->next_rand_time > timestamp){
                next_timestamp = (ota_handler->next_rand_time - timestamp) * 1000;
            }

            QM_LOGD(LOG_TAG,"next timestamp[%u], now timestamp[%u] except timer[%u]!!", ota_handler->next_rand_time, timestamp, next_timestamp);
            qm_post_delayed_action(&g_ota_handle->ota_notify_work, qm_iot_ota_start_aciton, NULL, next_timestamp);
        #endif

            progress_details = ((g_ota_handle->totalBytesReceived * 100) / g_ota_handle->documentFields.fileSize);

            qm_snprintf(client_token, 32, "%d", qm_now_ms());
            qm_snprintf(str_progress, QM_IOT_OTA_PROGRESS_STR_NUM, "%d%%", event->data.progress_update.progress_details);     
            qm_iot_jobs_update(ota_handler->did, strlen(ota_handler->did), ota_handler->jobs_id, strlen(ota_handler->jobs_id),
                                out_topic, TOPIC_MAX_STR_LEN, NULL);

            status = QM_IOT_JOBS_FAILED;
            request_len = qm_iot_jobs_update_msg(status, str_progress, strlen(str_progress),
                                                NULL, 0,
                                                client_token, strlen(client_token),
                                                request_data, QM_IOT_STREAMS_REQUEST_BUFFER_SIZE);
            if(request_len <= 0){
                break;
            }
            
            QM_LOGD(LOG_TAG, "pub[%s]:%s", out_topic, request_data);

            ota_handler->progress_details = event->data.progress_update.progress_details;
            qm_iot_mqtt_pub(ota_handler->mqtt_handle, out_topic, (uint8_t *)request_data, request_len, QM_MQTT_QoS0);

            if(ota_handler->event_handler){
                ota_handler->event_handler(ota_handler, event, ota_handler->user_data);
            }

        break;

        default:
        break;
    }

    event_type = event->type;
}

/*
{
    "f": 0,
    "i": 0,
    "l": 8,
    "p": "MTIzMTIzMTI="
}
*/

static qm_iot_streams_status_t ota_progress_data_match(int fileId, int blockId, int blockSize, int lastReceivedblockId)
{
    qm_iot_streams_status_t status = QM_IOT_STREAMS_STATUS_Success;
    
    // 验证参数范围
    if(fileId < 0 || blockId < 0 || blockSize < 0) {
        QM_LOGE(LOG_TAG, "Invalid parameters: fileId=%d, blockId=%d, blockSize=%d", fileId, blockId, blockSize);
        return QM_IOT_STREAMS_STATUS_DataDecodingFailed;
    }

    if( fileId != g_ota_handle->documentFields.fileId){
        /* Error - the file ID doesn't match with the one we received in the job document. */
        QM_LOGE(LOG_TAG, "ota fileId failed %d, expected %d", fileId, g_ota_handle->documentFields.fileId);
        status = QM_IOT_STREAMS_STATUS_DataDecodingFailed;
    }else if( blockSize > CONFIG_QM_IOT_STREAMS_BLOCK_SIZE ){
        /* Error - the block size doesn't match with what we requested. It can be smaller as
            * the last block may or may not be of exact size. */
        QM_LOGE(LOG_TAG, "ota blockSize failed %d, max allowed %d", blockSize, CONFIG_QM_IOT_STREAMS_BLOCK_SIZE);
        status = QM_IOT_STREAMS_STATUS_DataDecodingFailed;
    }else if( blockId <= lastReceivedblockId ){
        /* Ignore this block. */
        QM_LOGE(LOG_TAG, "ota blockId failed now %d last %d", blockId, lastReceivedblockId);
        status = QM_IOT_STREAMS_STATUS_DataDecodingFailed;        
    }

    return status;
} 

static void qm_iot_ota_update_progress_handler(void *handle, const qm_iot_mqtt_recv_t *packet, void *userdata)
{
    int fileId;
    int blockId;
    int blockSize;
    int progress_details;
    int decodedDataLength = 0;
    qm_iot_ota_event_t event = {0};
    static uint8_t decodedData[ CONFIG_QM_IOT_STREAMS_BLOCK_SIZE ] = {0};
    int result = -1;
    qm_iot_steams_context_t *streams_context = NULL;

    if(g_ota_handle == NULL || handle == NULL || packet == NULL){
        return ;
    }

    streams_context = &g_ota_handle->streams_context;
    memset(decodedData, 0, CONFIG_QM_IOT_STREAMS_BLOCK_SIZE);
    if(g_ota_handle->documentFields.fileSize == 0){
        return ;
    }

    result = (int)qm_iot_streams_recviced_datablock(streams_context,packet->data.pub.payload, packet->data.pub.payload_len,
                                                &fileId, &blockId, &blockSize, decodedData, &decodedDataLength );

    if( result != QM_IOT_STREAMS_STATUS_Success ){
        QM_LOGE(LOG_TAG, "ota status failed %d",result);
        goto __next;
    }

    result = ota_progress_data_match(fileId, blockId, blockSize, g_ota_handle->lastReceivedblockId);                                  
    if( result != QM_IOT_STREAMS_STATUS_Success ){
        QM_LOGE(LOG_TAG, "ota match failed %d",result);
        goto __next;
    }

    memset(&event, 0, sizeof(qm_iot_ota_event_t));
    event.type = QM_IOT_OTA_EVENT_DATA_INSTALLING;
    event.data.data_write.ota_data = decodedData;
    event.data.data_write.data_len = (uint16_t)decodedDataLength;
    qm_iot_event_handler(g_ota_handle, &event);

    g_ota_handle->currentBlockOffset++;
    g_ota_handle->totalBytesReceived += decodedDataLength;
    g_ota_handle->lastReceivedblockId = blockId;

__next:

    if(g_ota_handle->totalBytesReceived < g_ota_handle->documentFields.fileSize){
        memset(decodedData, 0, CONFIG_QM_IOT_STREAMS_BLOCK_SIZE);
        decodedDataLength = qm_iot_steams_create_request(QM_IOT_OTA_TYPE, 
                                                        g_ota_handle->currentFileId, CONFIG_QM_IOT_STREAMS_BLOCK_SIZE,
                                                        g_ota_handle->currentBlockOffset, NUM_OF_BLOCKS_REQUESTED, 
                                                        (char *)decodedData, CONFIG_QM_IOT_STREAMS_BLOCK_SIZE);

        qm_iot_mqtt_pub(g_ota_handle->mqtt_handle, streams_context->topicGetStream, decodedData, decodedDataLength, QM_MQTT_QoS0);
    }

    progress_details = ((g_ota_handle->totalBytesReceived * 100) / g_ota_handle->documentFields.fileSize);

    QM_LOGD(LOG_TAG, "qm_ota write (%d/%d)progress %d", g_ota_handle->totalBytesReceived,g_ota_handle->documentFields.fileSize,progress_details);
    
    memset(&event, 0, sizeof(qm_iot_ota_event_t));
    event.type = QM_IOT_OTA_EVENT_PROGRESS_UPADTE;
    event.data.progress_update.progress_details = progress_details;
    qm_iot_event_handler(g_ota_handle, &event);
}

static int convertSignatureToDER(qm_iot_ota_handle_t *ota_handle, qm_iot_jobs_file_t * jobFields )
{
    int ret = QM_EOK;

    ota_handle->signatureLen = DECODE_MAX_STR_LEN;
    ret = qm_utils_base64decode(( const uint8_t * ) jobFields->signature, jobFields->signatureLen, 
                            DECODE_MAX_STR_LEN, ota_handle->signature, &ota_handle->signatureLen);
 
    if( ret != QM_EOK ){
        return -QM_ERROR;
    }

    jobFields->signature = ( const char * ) ota_handle->signature;
    jobFields->signatureLen = ota_handle->signatureLen;

    return QM_EOK;
}

/**
 * note: 平台在设备OTA完成时会继续下方一个空josn的 topic通知
 *       此处在接收实际job文件时再进行取消 ota_success_work 和 ota_notify_work 的定时器操作
 * detail: 
 * 
 * 1]空jobs 
 *  {
 *      "timestamp": 1726048263
 *  }
 * 
 * 2]
 * {
    "timestamp": 1726047226,
    "execution": {
        "jobId": "AFR_OTA-test-jobs-for-ota-0011",
        "status": "QUEUED",
        "queuedAt": 1726047225,
        "lastUpdatedAt": 1726047225,
        "versionNumber": 1,
        "executionNumber": 1,
        "jobDocument": {
            "afr_ota": {
                "protocols": [
                    "MQTT"
                ],
                "streamname": "AFR_OTA-552bd5ea-ab85-4d37-8dea-d156f679a49c",
                "files": [
                    {
                        "filepath": "/device/updates",
                        "filesize": 8,
                        "fileid": 0,
                        "certfile": "/certificates/authcert.pem",
                        "sig-sha1-rsa": "thisIsMockCertificate"
                    }
                ]
            }
        }
    }
}
 */
static void qm_iot_ota_notify_next_handler(void *handle, const qm_iot_mqtt_recv_t *packet, void *userdata)
{
    int retry = 0;
    int type = 0;
    int ret = QM_EOK;
    int job_len = 0;
    const char *job_doc = NULL;
    char *root = NULL;
    int root_len = 0;
    char *jobsid = NULL;
    int jobsid_len = 0U;
    qm_iot_ota_event_t event = {0};
    qm_iot_ota_handle_t *ota_handle = (qm_iot_ota_handle_t *)userdata;

    qm_cancel_delayed_action(&ota_handle->retry_work);
#if CONFIG_QM_IOT_SLIENT_OTA_SUPPORT
    qm_cancel_delayed_action(&ota_handle->ota_notify_work); //防止多次调用
#endif
    memset(&ota_handle->documentFields, 0, sizeof(qm_iot_jobs_file_t));

    root = qm_json_get_value_by_name((char *)packet->data.pub.payload, packet->data.pub.payload_len, "execution", &root_len, &type);
    if(root == NULL){
        QM_LOGE(LOG_TAG, "execution find failed!!");
        return ;
    }

    jobsid = qm_json_get_value_by_name((char *)root, root_len, "jobId", &jobsid_len, &type);
    if(jobsid == NULL){
        QM_LOGE(LOG_TAG, "jobId find failed!!");
        return ;
    }
    
    // 添加边界检查
    if(jobsid_len >= sizeof(ota_handle->jobs_id)) {
        QM_LOGE(LOG_TAG, "jobId too long");
        return;
    }
    
    memset(ota_handle->jobs_id, 0, sizeof(ota_handle->jobs_id));
    memcpy(ota_handle->jobs_id, jobsid, jobsid_len);

    qm_iot_jobs_get_document((const char *)packet->data.pub.payload, packet->data.pub.payload_len, &job_doc, &job_len);  
    if(job_doc == NULL){
        QM_LOGE(LOG_TAG, "GET JOB DOC: Failed");
        return ;
    }
    QM_LOGD(LOG_TAG, "GET JOB DOC:%.*s",job_len, job_doc);

    ret = qm_iot_job_pop_files(job_doc, job_len, 0, &ota_handle->documentFields);  
    if(ret != QM_TRUE){
        QM_LOGE(LOG_TAG, "GET JOB FILED: Failed");
        return ;
    }

    ret = convertSignatureToDER(ota_handle, &ota_handle->documentFields);  
    if(ret != QM_EOK){
        QM_LOGE(LOG_TAG, "GET JOB signature FILED: Failed");
        return ;
    }

    QM_LOGD(LOG_TAG, "GET fileId:%d fileSize %d",ota_handle->documentFields.fileId, ota_handle->documentFields.fileSize);
    if(ota_handle->documentFields.signature){
        QM_HEX_LOGD(LOG_TAG, "GET signature: ", ota_handle->documentFields.signature, ota_handle->documentFields.signatureLen);
    }

    if(ota_handle->documentFields.filepath){
        QM_LOGD(LOG_TAG, "GET filepath:%.*s",ota_handle->documentFields.filepathLen, ota_handle->documentFields.filepath);
    }

    if(ota_handle->documentFields.certfile){
        QM_LOGD(LOG_TAG, "GET certfile:%.*s",ota_handle->documentFields.certfileLen, ota_handle->documentFields.certfile);
    }

    if(ota_handle->documentFields.imageRef){
        QM_LOGD(LOG_TAG, "GET imageRef:%.*s",ota_handle->documentFields.imageRefLen, ota_handle->documentFields.imageRef);
    }

    qm_iot_steams_init(&ota_handle->streams_context, 
                        ota_handle->documentFields.imageRef, ota_handle->documentFields.imageRefLen, 
                        ota_handle->did, strlen(ota_handle->did),
                        QM_IOT_OTA_TYPE);

    ota_handle->currentBlockOffset = 0;
    ota_handle->totalBytesReceived = 0;
    ota_handle->currentFileId = ota_handle->documentFields.fileId;

    qm_cancel_delayed_action(&ota_handle->ota_notify_work);  //防止多次调用
    qm_cancel_delayed_action(&ota_handle->ota_success_work); //防止多次调用

    do{
        ret = qm_iot_mqtt_sub_wait_ack(ota_handle->mqtt_handle, ota_handle->streams_context.topicStreamData, 
                                        qm_iot_ota_update_progress_handler, QM_MQTT_QoS1);
        if(ret == QM_EOK){
            break;
        }
    }while(retry++ <CONFIG_QM_IOT_OTA_RETRY_COUNT);
    
    
    event.type = QM_IOT_OTA_EVENT_START;
    event.data.start.topic_getstream = ota_handle->streams_context.topicGetStream;
    qm_iot_event_handler(ota_handle, &event);
}

/**
 * @brief 结束ota会话, 销毁实例并回收资源
 *
 * @param[in] handle 指向ota会话句柄的指针
 *
 * @return int32_t
 * @retval <QM_EOK  执行失败
 * @retval >=QM_EOK 执行成功
 * 
 */
int32_t qm_iot_ota_deinit(void **handle)
{
    if(handle == NULL || *handle == NULL){
        return -QM_EINVAL;
    }

    qm_iot_ota_handle_t *ota_handle = (qm_iot_ota_handle_t *)(*handle);
    
    qm_mutex_lock(&g_ota_global_lock, QM_WAIT_FOREVER);
    
    qm_iot_crypto_deinit(&ota_handle->crypto_handle);
    
    // 清理实例锁
    if(qm_mutex_is_valid(&ota_handle->ota_lock)) {
        qm_mutex_free(&ota_handle->ota_lock);
    }
    
    qm_free(ota_handle);
    
    if(g_ota_handle == ota_handle) {
        g_ota_handle = NULL;
    }
    
    *handle = NULL;
    
    qm_mutex_unlock(&g_ota_global_lock);

    return QM_EOK;
}

/**
 * @brief 创建ota会话实例, 并以默认值配置会话参数
 *
 * @return void *
 * @retval 非NULL provision实例的句柄
 * @retval NULL   初始化失败, 一般是内存分配失败导致
 * 
 */
void *qm_iot_ota_init(void)
{
    uint32_t did = 0;
    
    // 初始化全局锁
    static bool_t global_lock_init = QM_FALSE;
    if(!global_lock_init) {
        qm_mutex_new(&g_ota_global_lock);
        global_lock_init = QM_TRUE;
    }
    
    qm_mutex_lock(&g_ota_global_lock, QM_WAIT_FOREVER);
    
    if(g_ota_handle != NULL) {
        qm_mutex_unlock(&g_ota_global_lock);
        return g_ota_handle;  // 避免重复初始化
    }
    
    qm_iot_ota_handle_t *ota_handle = (qm_iot_ota_handle_t *)qm_malloc(sizeof(qm_iot_ota_handle_t));
    if(ota_handle == NULL){
        qm_mutex_unlock(&g_ota_global_lock);
        return NULL;
    }
    
    memset(ota_handle, 0, sizeof(qm_iot_ota_handle_t));
    
    // 初始化实例锁
    if(qm_mutex_new(&ota_handle->ota_lock) != QM_EOK) {
        qm_free(ota_handle);
        qm_mutex_unlock(&g_ota_global_lock);
        return NULL;
    }

    ota_handle->crypto_handle = qm_iot_crypto_init();
    if(ota_handle->crypto_handle == NULL){
        qm_mutex_free(&ota_handle->ota_lock);
        qm_free(ota_handle);
        qm_mutex_unlock(&g_ota_global_lock);
        return NULL;
    }

    did = qm_iot_did_get();
    ota_handle->pid = qm_iot_pid_get();

    int ret = qm_snprintf(ota_handle->did, CONFIG_QM_IOT_DID_MAX_LEN, "%d", did);
    if(ret >= CONFIG_QM_IOT_DID_MAX_LEN) {
        QM_LOGE(LOG_TAG, "DID string too long");
    }
    
    g_ota_handle = ota_handle;
    qm_mutex_unlock(&g_ota_global_lock);

    return (void *)ota_handle;
}

static void qm_iot_notify_ota_handler(void *handle, const qm_iot_mqtt_recv_t *packet, void *userdata)
{
    int type = 0;
    char *params = NULL;
    int params_len = 0;
    char *payload = NULL;
    int payload_len = 0;
    int msg_id = 0;
    uint32_t timestamp = 0;
    qm_iot_ota_event_t event = {0};
    char topic[TOPIC_MAX_STR_LEN + 1] = {0};
    char post_payload[DECODE_MAX_STR_LEN + 1] = {0};

    QM_LOGD(LOG_TAG, "recv ota notify %.*s", packet->data.pub.payload_len, packet->data.pub.payload);

    payload = qm_json_get_value_by_name((char *)packet->data.pub.payload, packet->data.pub.payload_len, ID_KEY, &payload_len, &type);
    if(payload == NULL){
        QM_LOGE(LOG_TAG, "msg_id find failed!!");
        goto __error;
    }
    msg_id = int_str_to_num(payload, payload_len);
    
    params = qm_json_get_value_by_name((char *)packet->data.pub.payload, packet->data.pub.payload_len, PARAMS_KEY, &params_len, &type);
    if(params == NULL){
        QM_LOGE(LOG_TAG, "params find failed!!");
        goto __error;
    }

    payload = qm_json_get_value_by_name(params, params_len, TYPE_KEY, &payload_len, &type);
    if(payload == NULL){
        QM_LOGE(LOG_TAG, "type find failed!!");
        goto __error;
    }
    event.data.ready.ota_type = int_str_to_num(payload, payload_len);
    
    payload = qm_json_get_value_by_name(params, params_len, VESION_KEY, &payload_len, &type);
    if(payload == NULL){
        QM_LOGE(LOG_TAG, "version find failed!!");
        goto __error;
    }
    event.data.ready.update_verion = int_str_to_num(payload, payload_len);
    qm_cancel_delayed_action(&g_ota_handle->ota_notify_work);
    
    event.type = QM_IOT_OTA_EVENT_READY;
    qm_iot_event_handler(g_ota_handle, &event);

    timestamp = qm_time(NULL);
    int ret1 = qm_snprintf(topic, TOPIC_MAX_STR_LEN, OTA_NOTIFY_REPLY_TOPIC, g_ota_handle->pid, g_ota_handle->did);
    int ret2 = qm_snprintf(post_payload, DECODE_MAX_STR_LEN, OTA_NOTIFY_REPLY_PAYLOAD, msg_id, event.data.ready.result, timestamp);
    
    if(ret1 >= TOPIC_MAX_STR_LEN || ret2 >= DECODE_MAX_STR_LEN) {
        QM_LOGE(LOG_TAG, "OTA notify message too long");
        return;
    }

    QM_LOGD(LOG_TAG, "OTA Notify ack pub[%s]:%s!!", topic, post_payload);
    qm_iot_mqtt_pub(g_ota_handle->mqtt_handle, topic, (uint8_t *)post_payload, strlen(post_payload), QM_MQTT_QoS0);

    return ;

__error:
    event.type = QM_IOT_OTA_EVENT_CANCEL;
    qm_iot_event_handler(g_ota_handle, &event);
}

static void qm_iot_ota_start_aciton(void *arg)
{
    uint32_t timestamp = qm_time(NULL);
    char topic[TOPIC_MAX_STR_LEN + 1] = {0};
    char payload[DECODE_MAX_STR_LEN + 1] = {0};

    int ret1 = qm_snprintf(payload, DECODE_MAX_STR_LEN, OTA_START_PAYLOAD, 0, timestamp);
    int ret2 = qm_snprintf(topic, TOPIC_MAX_STR_LEN, OTA_START_TOPIC, g_ota_handle->pid, g_ota_handle->did);
    
    if(ret1 >= DECODE_MAX_STR_LEN || ret2 >= TOPIC_MAX_STR_LEN) {
        QM_LOGE(LOG_TAG, "OTA start message too long");
        return;
    }

    QM_LOGD(LOG_TAG, "OTA start pub[%s]:%s!!", topic, payload);
    qm_iot_mqtt_pub(g_ota_handle->mqtt_handle, topic, (uint8_t *)payload, strlen(payload), QM_MQTT_QoS0);
}

static void qm_iot_start_ota_reply_notify_handler(void *handle, const qm_iot_mqtt_recv_t *packet, void *userdata)
{
    int type = 0;
    char *params = NULL;
    int params_len = 0;
    char *payload = NULL;
    int payload_len = 0;
    int otaState = 0;
    int index = 0;
    uint32_t time_array[2] = {0}; // start + end
    uint32_t timestamp = 0;
    uint32_t next_timestamp = 0;
    qm_iot_ota_event_t event = {0};
    qm_iot_ota_handle_t *ota_handle = (qm_iot_ota_handle_t *)userdata;

    QM_LOGD(LOG_TAG, "recv version ack %.*s", packet->data.pub.payload_len, packet->data.pub.payload);
    params = qm_json_get_value_by_name((char *)packet->data.pub.payload, packet->data.pub.payload_len, PARAMS_KEY, &params_len, &type);
    if(params == NULL){
        QM_LOGE(LOG_TAG, "params find failed!!");
        return ;
    }

#if CONFIG_QM_IOT_SLIENT_OTA_SUPPORT
    payload = qm_json_get_value_by_name(params, params_len, START_TIME_KEY, &payload_len, &type);
    if(payload == NULL){
        QM_LOGE(LOG_TAG, "start time find failed!!");
        return ;
    }
    time_array[index] = int_str_to_num(payload, payload_len);
    if(time_array[index] <= TIMESTAMP_2000){
        return ;
    }

    index++;
    payload = qm_json_get_value_by_name(params, params_len, END_TIME_KEY, &payload_len, &type);
    if(payload == NULL){
        QM_LOGE(LOG_TAG, "end time find failed!!");
        return ;
    }
    time_array[index] = int_str_to_num(payload, payload_len);
    if(time_array[index] <= TIMESTAMP_2000){
        return ;
    }

    qm_cancel_delayed_action(&g_ota_handle->ota_notify_work);

    qm_srandom(qm_now_ms());
    ota_handle->next_rand_time = time_array[0] + qm_random_get(time_array[1] - time_array[0]);
 
    timestamp = qm_time(NULL);
    next_timestamp = (ota_handle->next_rand_time - timestamp) * 1000;

    QM_LOGD(LOG_TAG,"next timestamp[%u], now timestamp[%u] except timer[%u]!!", ota_handle->next_rand_time, timestamp, next_timestamp);
    qm_post_delayed_action(&g_ota_handle->ota_notify_work, qm_iot_ota_start_aciton, NULL, next_timestamp);
#endif

    payload = qm_json_get_value_by_name(params, params_len, OTA_STATE_KEY, &payload_len, &type);
    if(payload == NULL){
        QM_LOGE(LOG_TAG, "otaState find failed!!");
        return ;
    }
    otaState = int_str_to_num(payload, payload_len);
    if(otaState != QM_IOT_CODE_SUCCESS){
        QM_LOGW(LOG_TAG, "otaState[%u]!!", otaState);
        return ;
    }

    qm_cancel_delayed_action(&g_ota_handle->ota_notify_work);

    payload = qm_json_get_value_by_name(params, params_len, TYPE_KEY, &payload_len, &type);
    if(payload == NULL){
        QM_LOGE(LOG_TAG, "type find failed!!");
        return ;
    }
    event.data.ready.ota_type = int_str_to_num(payload, payload_len);
    
    payload = qm_json_get_value_by_name(params, params_len, VESION_KEY, &payload_len, &type);
    if(payload == NULL){
        QM_LOGE(LOG_TAG, "version find failed!!");
        return ;
    }
    event.data.ready.update_verion = int_str_to_num(payload, payload_len);
    
    event.type = QM_IOT_OTA_EVENT_READY;
    qm_iot_event_handler(g_ota_handle, &event);
}

/**
 * @brief 配置ota会话
 *
 * @param[in] handle ota会话句柄
 * @param[in] option 配置选项, 更多信息请参考@ref qm_iot_ota_option_t
 * @param[in] data   配置选项数据, 更多信息请参考@ref qm_iot_ota_option_t
 *
 * @return int32_t
 * @retval =QM_EOK 执行成功, <QM_EOK 执行失败
 *
 */
int32_t qm_iot_ota_setopt(void *handle, qm_iot_ota_option_t option, void *data)
{
    int ret = QM_EOK;
    char out_topic[TOPIC_MAX_STR_LEN + 1] = {0};
    qm_iot_ota_handle_t *ota_handle = (qm_iot_ota_handle_t *)handle;
    if(ota_handle == NULL || data == NULL){
        return -QM_EINVAL;
    }

    switch (option)
    {
        case QM_IOT_OTAOPT_MQTT_HANDLE:

            ota_handle->mqtt_handle = data;
        
        #if 0
            qm_snprintf(out_topic, TOPIC_MAX_STR_LEN, OTA_VERSION_INFORM_REPLAY_TOPIC, g_ota_handle->pid, g_ota_handle->did);
            qm_iot_mqtt_pre_sub(g_ota_handle->mqtt_handle, out_topic, qm_iot_ota_version_ack_notify_handler, QM_MQTT_QoS1, ota_handle);
        #endif
            memset(out_topic, 0, TOPIC_MAX_STR_LEN);
            qm_iot_jobs_get_topic(QM_IOT_JOBS_TOPIC_NEX_CHANGED, ota_handle->did, strlen(ota_handle->did), 
                                    NULL, 0, out_topic, TOPIC_MAX_STR_LEN, NULL);
            qm_iot_mqtt_pre_sub(g_ota_handle->mqtt_handle, out_topic, qm_iot_ota_notify_next_handler, QM_MQTT_QoS0, ota_handle);

            memset(out_topic, 0, TOPIC_MAX_STR_LEN);
            qm_snprintf(out_topic, TOPIC_MAX_STR_LEN, OTA_NOTIFY_TOPIC, g_ota_handle->pid, g_ota_handle->did);
            qm_iot_mqtt_pre_sub(g_ota_handle->mqtt_handle, out_topic, qm_iot_notify_ota_handler, QM_MQTT_QoS0, ota_handle);

            memset(out_topic, 0, TOPIC_MAX_STR_LEN);
            qm_snprintf(out_topic, TOPIC_MAX_STR_LEN, OTA_START_REPLY_TOPIC, g_ota_handle->pid, g_ota_handle->did);
            qm_iot_mqtt_pre_sub(g_ota_handle->mqtt_handle, out_topic, qm_iot_start_ota_reply_notify_handler, QM_MQTT_QoS0, ota_handle);
        break;

        case QM_IOT_OTAOPT_SINGATURE:
            ota_handle->pucSignature = (char *)data;
        break;
        case QM_IOT_OTAOPT_EVENT_HANDLER:
            ota_handle->event_handler = (qm_iot_ota_event_handler_t)data;
        break;

        case QM_IOT_OTAOPT_USERDATA:
            ota_handle->user_data = data;
        break;

        default:
            ret = -QM_EINVAL;
        break;
    }

    return ret;
}

/**
 * @brief 暂停ota流程
 *
 * @param handle dynreg会话句柄
 *
 * @return int32_t
 * @retval <QM_EOK  数据接收失败
 * @retval >=QM_EOK 数据接收成功
 */
int32_t qm_iot_ota_stop(void *handle)
{
    qm_iot_ota_handle_t *ota_handle = (qm_iot_ota_handle_t *)handle;
    if(ota_handle == NULL){
        return -QM_EINVAL;
    }

    qm_iot_mqtt_unsub(g_ota_handle->mqtt_handle, g_ota_handle->streams_context.topicStreamData);

    qm_cancel_delayed_action(&g_ota_handle->retry_work);
    qm_cancel_delayed_action(&g_ota_handle->ota_notify_work);

    return QM_EOK;
}

/**
 * @brief 启动ota流程
 *
 * @param handle dynreg会话句柄
 *
 * @return int32_t
 * @retval <QM_EOK  数据接收失败
 * @retval >=QM_EOK 数据接收成功
 */
int32_t qm_iot_ota_start(void *handle)
{
    uint32_t timestamp = qm_time(NULL);
    uint32_t version = qm_iot_version_get();
    char topic[TOPIC_MAX_STR_LEN + 1] = {0};
    char payload[DECODE_MAX_STR_LEN + 1] = {0};
    qm_iot_ota_handle_t *ota_handle = (qm_iot_ota_handle_t *)handle;
    if(ota_handle == NULL){
        return -QM_EINVAL;
    }

    qm_cancel_delayed_action(&ota_handle->ota_notify_work);

    if(ota_handle->msg_id){
        QM_LOGW(LOG_TAG, "OTA Ver retry!!");
    }

    ota_handle->msg_id++;
    int ret1 = qm_snprintf(topic, TOPIC_MAX_STR_LEN, OTA_VERSION_INFORM_TOPIC, g_ota_handle->pid, g_ota_handle->did);
    int ret2 = qm_snprintf(payload, DECODE_MAX_STR_LEN, OTA_VERSION_INFORM_PAYLOAD, ota_handle->msg_id, CONFIG_QM_IOT_SDK_VESRION, version, CONFIG_QM_IOT_SLIENT_OTA_SUPPORT, timestamp);
    
    if(ret1 >= TOPIC_MAX_STR_LEN || ret2 >= DECODE_MAX_STR_LEN) {
        QM_LOGE(LOG_TAG, "OTA version message too long");
        return -QM_EINVAL;
    }

    QM_LOGD(LOG_TAG, "OTA Verr pub[%s]:%s!!", topic, payload);
    qm_iot_mqtt_pub(g_ota_handle->mqtt_handle, topic, (uint8_t *)payload, strlen(payload), QM_MQTT_QoS0);

    return qm_post_delayed_action(&g_ota_handle->ota_notify_work, qm_iot_ota_start_aciton, NULL, 1000);
}
