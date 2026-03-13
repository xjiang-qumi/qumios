#include "qm.h"
#include "nc_audio.h"

#include "comm_base.h"

#include "qm_work.h"

typedef struct 
{
    int playload_len;
    uint8_t *playload;
}nc_audio_send_data_t;

typedef struct 
{
    qm_mutex_t lock;
    uint16_t need_len;
    nc_audio_event_t event_notify;
    nc_audio_fmt_type_t           fmt_type;          /*!< 音频格式*/
    uint16_t                      rate;              /*!< 音频采样率*/
    uint16_t                      bits;              /*!< 音频采样位数*/
    uint16_t                      channel;           /*!< 音频声道数*/
    uint8_t                       mic_gain;
    uint8_t                       spk_gain;
    uint8_t                       vad_timeout;
}nc_audio_ctx_t;

static nc_audio_ctx_t *g_comm_ctx = NULL;

static int comm_base_check(uint8_t *data, int len);
static int comm_base_sub_cmd_get(uint8_t *data, int len);
static int comm_base_id_get(uint8_t *data, int len);
static int comm_base_notify(comm_base_event_info_t *event_info);

static int header1_recv_len_cb(void);
static int header1_recv_check(uint8_t *data, int len);

static int header2_recv_len_cb(void);
static int header2_recv_check(uint8_t *data, int len);

static int ver_recv_len_cb(void);
static int ver_recv_check(uint8_t *data, int len);

static int cmd_recv_len_cb(void);
static int cmd_recv_check(uint8_t *data, int len);

static int len_recv_len_cb(void);
static int len_recv_check(uint8_t *data, int len);

static int data_recv_len_cb(void);
static int data_recv_check(uint8_t *data, int len);

static int mcu_get_ver_unpack(uint8_t *data, int len);
static int mcu_get_ver_pack(uint8_t *data, int *len, void *arg);

static int mic_cfg_unpack(uint8_t *data, int len);
static int mic_cfg_pack(uint8_t *data, int *len, void *arg);

static int spk_cfg_unpack(uint8_t *data, int len);
static int spk_cfg_pack(uint8_t *data, int *len, void *arg);

static int mic_data_unpack(uint8_t *data, int len);
static int mic_data_pack(uint8_t *data, int *len, void *arg);

static int spk_play_unpack(uint8_t *data, int len);
static int spk_play_pack(uint8_t *data, int *len, void *arg);

static int wakeup_unpack(uint8_t *data, int len);
static int wakeup_pack(uint8_t *data, int *len, void *arg);

static int sleep_unpack(uint8_t *data, int len);
static int sleep_pack(uint8_t *data, int *len, void *arg);

static int cfg_unpack(uint8_t *data, int len);
static int cfg_pack(uint8_t *data, int *len, void *arg);

static int record_func_unpack(uint8_t *data, int len);
static int record_func_pack(uint8_t *data, int *len, void *arg);

static int spk_reply_unpack(uint8_t *data, int len);
static int spk_reply_pack(uint8_t *data, int *len, void *arg);

static uint8_t frame_chksum(uint8_t *buffer, uint16_t length);
static int32_t fram_chk_verify(uint8_t *buffer, uint16_t length);
static int frame_pack(int sub_cmd, nc_audio_frame_header_t *header, uint8_t *data, int len);


int32_t nc_audio_event_register(nc_audio_event_t event_notify)
{
    if(g_comm_ctx == NULL){
        return -QM_EINVAL;
    }

    g_comm_ctx->event_notify = event_notify;

    return QM_EOK;
}

int32_t nc_audio_set_spk_volume(uint8_t value)
{   
    nc_audio_spk_cfg_t spk_cfg = {0};
    nc_audio_send_data_t send_data = {0};

    g_comm_ctx->spk_gain = value;

    spk_cfg.bit = g_comm_ctx->bits;
    spk_cfg.type = g_comm_ctx->fmt_type;
    spk_cfg.rate  = cpu_to_be32(g_comm_ctx->rate);
    spk_cfg.size  = cpu_to_be32(CONFIG_NC_AUDIO_RECV_MAX_SIZE);
    spk_cfg.spk_gain = g_comm_ctx->spk_gain;


    send_data.playload = (uint8_t *)&spk_cfg;
    send_data.playload_len = sizeof(nc_audio_spk_cfg_t);
    comm_base_cmd_send(NC_AUDIO_SUB_CMD_SPK_CFG, CONFIG_NC_AUDIO_SEND_COUNT, CONFIG_NC_AUDIO_SEND_TIMEOUT_MS, (void *)&send_data);
    
    return QM_EOK;
}

int32_t nc_audio_cfg(nc_audio_cfg_type_t type, int value)
{
    nc_audio_cfg_t cfg = {0};
    nc_audio_ext_cfg_t ext_cfg = {0};

    nc_audio_send_data_t send_data = {0};

    if(value > 0xFF){
        ext_cfg.type = type;
        ext_cfg.value = value;
        ext_cfg.value = cpu_to_be32(ext_cfg.value);
        send_data.playload = (uint8_t *)&ext_cfg;
        send_data.playload_len = sizeof(nc_audio_ext_cfg_t);
    }else{
        cfg.type = type;
        cfg.value = value;
        send_data.playload = (uint8_t *)&cfg;
        send_data.playload_len = sizeof(nc_audio_cfg_t);
    }

    comm_base_cmd_send(NC_AUDIO_SUB_CMD_AUDIO_CFG, CONFIG_NC_AUDIO_SEND_COUNT, CONFIG_NC_AUDIO_SEND_TIMEOUT_MS, (void *)&send_data);

    return QM_EOK;
}

int32_t nc_audio_record_func(nc_audio_record_func_t func)
{

    nc_audio_send_data_t send_data = {0};

    send_data.playload = (uint8_t *)&func;
    send_data.playload_len = sizeof(nc_audio_record_func_t);

    comm_base_cmd_send(NC_AUDIO_SUB_CMD_RECORD_FUNC, CONFIG_NC_AUDIO_SEND_COUNT, CONFIG_NC_AUDIO_SEND_TIMEOUT_MS, (void *)&send_data);

    return QM_EOK;
}

extern int seial_send(uint8_t *data, int len);
int32_t nc_play_spk_direct_play(nc_audio_spk_play_header_t *play, uint8_t *payload, int payload_len)
{
    nc_audio_send_data_t send_data = {0};
    int len = 0;
    int buf_len = 0;
    uint8_t *buf = NULL;
    uint8_t *pack_buf = NULL;
    qm_err_t ret = QM_EOK;
    nc_audio_spk_play_t *spk_ply = (nc_audio_spk_play_t *)qm_malloc(sizeof(nc_audio_spk_play_t) + payload_len);
    if(spk_ply == NULL){
        ret = -QM_ENOMEM;
        goto __exit;
    }

    send_data.playload = (uint8_t *)spk_ply;
    send_data.playload_len = sizeof(nc_audio_spk_play_t) + payload_len - 1;

    spk_ply->type = play->type;
    spk_ply->msg_id = cpu_to_be32(play->msg_id);

    if(payload_len){
        memcpy(spk_ply->playload, payload, payload_len);
    }

    ret = spk_play_pack(NULL, &len, &send_data);
    if(ret != QM_EOK){
        goto __exit;
    }

    buf_len = len;

    buf = (uint8_t*)qm_malloc(buf_len);
    if(buf == NULL){
        ret = -QM_ENOMEM;
        goto __exit;
    }

    pack_buf = buf;

    ret = spk_play_pack(pack_buf, &len, &send_data);

    if(ret != QM_EOK){
        goto __exit;
    }
    
    seial_send(buf, buf_len);

__exit:

    if(buf){
        qm_free(buf);
        buf = NULL;
    }

    if(spk_ply){
        qm_free(spk_ply);
        spk_ply = NULL;
    }

    return ret;
}

int32_t nc_audio_spk_play(nc_audio_spk_play_header_t *play, uint8_t *payload, int payload_len)
{
    nc_audio_send_data_t send_data = {0};
    nc_audio_spk_play_t *spk_ply = (nc_audio_spk_play_t *)qm_malloc(sizeof(nc_audio_spk_play_t) + payload_len);
    if(spk_ply == NULL){
        return -QM_ENOMEM;
    }

    send_data.playload = (uint8_t *)spk_ply;
    send_data.playload_len = sizeof(nc_audio_spk_play_t) + payload_len - 1;

    spk_ply->type = play->type;
    spk_ply->msg_id = cpu_to_be32(play->msg_id);

    if(payload_len){
        memcpy(spk_ply->playload, payload, payload_len);
    }
    int ret = comm_base_cmd_send(NC_AUDIO_SUB_CMD_SPK_PLAY, CONFIG_NC_AUDIO_SEND_COUNT, CONFIG_NC_AUDIO_SEND_TIMEOUT_MS, (void *)&send_data);
    if(ret != QM_EOK) {
        QM_LOGD("1", "nc_audio_spk_play error:%d", ret);
    }
    if(spk_ply){
        qm_free(spk_ply);
        spk_ply = NULL;
    }

    return QM_EOK;
}

int32_t nc_audio_recv_push(uint8_t *data, int len)
{
    return comm_base_data_push(data, len);
}

int32_t nc_audio_init(nc_audio_param_t *param)
{  
    comm_base_param_t base_param = {0};
    nc_audio_send_data_t send_data = {0};

    g_comm_ctx = (nc_audio_ctx_t*)qm_malloc(sizeof(nc_audio_ctx_t));
    if(NULL == g_comm_ctx){
       return -QM_ENOMEM;
    }    
    memset(g_comm_ctx, 0, sizeof(nc_audio_ctx_t));

    g_comm_ctx->need_len = 1;
    g_comm_ctx->event_notify = param->notify;
    g_comm_ctx->rate = param->rate;
    g_comm_ctx->bits = param->bits;
    g_comm_ctx->channel = param->channel;
    g_comm_ctx->fmt_type = param->fmt_type;
    g_comm_ctx->mic_gain = param->mic_gain;
    g_comm_ctx->spk_gain = param->spk_gain;
    g_comm_ctx->vad_timeout = param->vad_tiemout;

    base_param.recv_timeout = CONFIG_NC_AUDIO_RECV_TIMEOUT_MS;
    base_param.send = param->send;
    base_param.check = comm_base_check;
    base_param.id_get = comm_base_id_get;
    base_param.notify = comm_base_notify;
    base_param.cmd_get = comm_base_sub_cmd_get;

#if CONFIG_QM_OS_SUPPORT
    qm_mutex_new(&g_comm_ctx->lock);
#endif
        
    comm_base_init(&base_param);

    comm_base_recv_register(NC_AUDIO_PARSE_STATE_HEAD_1, header1_recv_len_cb, header1_recv_check);
    comm_base_recv_register(NC_AUDIO_PARSE_STATE_HEAD_2, header2_recv_len_cb, header2_recv_check);
    comm_base_recv_register(NC_AUDIO_PARSE_STATE_VER, ver_recv_len_cb, ver_recv_check);
    comm_base_recv_register(NC_AUDIO_PARSE_STATE_CMD, cmd_recv_len_cb, cmd_recv_check);
    comm_base_recv_register(NC_AUDIO_PARSE_STATE_LEN, len_recv_len_cb, len_recv_check);
    comm_base_recv_register(NC_AUDIO_PARSE_STATE_DATA, data_recv_len_cb, data_recv_check);
    
    comm_base_cmd_register(NC_AUDIO_SUB_CMD_AUDIO_WAKEUP, COMM_BASE_DIR_UP, wakeup_pack, wakeup_unpack);
    comm_base_cmd_register(NC_AUDIO_SUB_CMD_GET_MCU_VER, COMM_BASE_DIR_UP, mcu_get_ver_pack, mcu_get_ver_unpack);
    comm_base_cmd_register(NC_AUDIO_SUB_CMD_MIC_CFG, COMM_BASE_DIR_UP, mic_cfg_pack, mic_cfg_unpack);
    comm_base_cmd_register(NC_AUDIO_SUB_CMD_SPK_CFG, COMM_BASE_DIR_UP, spk_cfg_pack, spk_cfg_unpack);
    comm_base_cmd_register(NC_AUDIO_SUB_CMD_MIC_DATA, COMM_BASE_DIR_UP, mic_data_pack, mic_data_unpack);
    comm_base_cmd_register(NC_AUDIO_SUB_CMD_SPK_PLAY, COMM_BASE_DIR_UP, spk_play_pack, spk_play_unpack);
    comm_base_cmd_register(NC_AUDIO_SUB_CMD_AUDIO_SLEEP, COMM_BASE_DIR_UP, sleep_pack, sleep_unpack);
    comm_base_cmd_register(NC_AUDIO_SUB_CMD_AUDIO_CFG, COMM_BASE_DIR_UP, cfg_pack, cfg_unpack);
    comm_base_cmd_register(NC_AUDIO_SUB_CMD_RECORD_FUNC, COMM_BASE_DIR_UP, record_func_pack, record_func_unpack);
    comm_base_cmd_register(NC_AUDIO_SUB_CMD_SKP_REPLY, COMM_BASE_DIR_UP, spk_reply_pack, spk_reply_unpack);

    qm_msleep(100);
    comm_base_cmd_send(NC_AUDIO_SUB_CMD_GET_MCU_VER, CONFIG_NC_AUDIO_SEND_COUNT, CONFIG_NC_AUDIO_SEND_TIMEOUT_MS, (void *)&send_data);

    return QM_EOK;
}

static int header1_recv_len_cb(void)
{
    return 1;
}

static int header1_recv_check(uint8_t *data, int len)
{
    if(data == NULL || len == 0){
        return -QM_EINVAL;
    }

    if(data[0] != NC_AUDIO_FRAME_HEADER_1){
        return -QM_ERROR;
    }
 
    return QM_EOK;
}

static int header2_recv_len_cb(void)
{
    return 1;
}

static int header2_recv_check(uint8_t *data, int len)
{
    if(data == NULL || len == 0){
        return -QM_EINVAL;
    }

    if(data[0] != NC_AUDIO_FRAME_HEADER_2){
        return -QM_ERROR;
    }
 
    return QM_EOK;
}

static int ver_recv_len_cb(void)
{
    return 1;
}

static int ver_recv_check(uint8_t *data, int len)
{
    if(data == NULL || len != 1){
        return -QM_EINVAL;
    }

    if(data[0] != NC_AUDIO_FRAME_VER){
        return -QM_ERROR;
    }
 
    return QM_EOK;
}


static int cmd_recv_len_cb(void)
{
    return 1;
}

static int cmd_recv_check(uint8_t *data, int len)
{
    if(data == NULL || len != 1){
        return -QM_EINVAL;
    }

    if(data[0] != NC_AUDIO_FRAME_CMD){
        return -QM_ERROR;
    }
 
    return QM_EOK;
}

static int len_recv_len_cb(void)
{
    return 2;
}

static int len_recv_check(uint8_t *data, int len)
{
    if(data == NULL){
        return -QM_EINVAL;
    }

    g_comm_ctx->need_len = ((data[0] << 8) | data[1]);

    // ADD8 
    g_comm_ctx->need_len += sizeof(uint8_t);
    
    return QM_EOK;
}

static int data_recv_len_cb(void)
{
    return g_comm_ctx->need_len;
}

static int data_recv_check(uint8_t *data, int len)
{
    if(data == NULL || len != g_comm_ctx->need_len){
        return -QM_EINVAL;
    }

    g_comm_ctx->need_len = 1;

    return QM_EOK;
}

static int comm_base_check(uint8_t *data, int len)
{
    if (data == NULL || len < NC_AUDIO_FRAME_MIN_LEN){
        return -QM_EINVAL;
    }

    // QM_HEX_LOGD("1", "recv ", data, len);

    return fram_chk_verify(data, len);
}

static int comm_base_sub_cmd_get(uint8_t *data, int len)
{
    if (data == NULL || len < NC_AUDIO_FRAME_MIN_LEN){
        return -QM_EINVAL;
    }

    return (data[NC_AUDIO_FRAME_SUB_CMD_ADDR]);
}

static int comm_base_id_get(uint8_t *data, int len)
{
    return QM_EOK;
}

static int comm_base_notify(comm_base_event_info_t *event_info)
{
    return QM_EOK;
}

static uint8_t frame_chksum(uint8_t *buffer, uint16_t length)
{
    uint16_t i;
    uint8_t check_sum = 0;
    for (i = 0; i < length; i++){
        check_sum += buffer[i];
    }
    return check_sum;
}

static int32_t fram_chk_verify(uint8_t *buffer, uint16_t length)
{
    uint8_t check_sum = frame_chksum(buffer, length - 1);
    if(check_sum != buffer[length - 1]){
        return -QM_ERROR;
    }

    return QM_EOK;
}   

static int frame_pack(int sub_cmd, nc_audio_frame_header_t *header, uint8_t *data, int len)
{
    if(header == NULL ){
        return -QM_EINVAL;
    }

    header->head_1 = NC_AUDIO_FRAME_HEADER_1;
    header->head_2 = NC_AUDIO_FRAME_HEADER_2;
    header->ver = NC_AUDIO_FRAME_VER_1;
    header->cmd = NC_AUDIO_FRAME_CMD;
    header->len = be16_to_cpu(len+ 1);
    header->sub_cmd = sub_cmd;

    if(len){
        memcpy(header->payload, data, len);
    }

    return QM_EOK;
}

static int mcu_get_ver_unpack(uint8_t *data, int len)
{
    uint16_t payload_len = 0;
    nc_audio_event_param_t event = {0};
    nc_audio_send_data_t send_data = {0};
    nc_audio_mic_cfg_t mic_cfg = {0};
    nc_audio_spk_cfg_t spk_cfg = {0};
    nc_audio_get_mcu_ver_t *mcu_ver = NULL;
    nc_audio_frame_header_t *header = (nc_audio_frame_header_t *)data;

    mcu_ver = &event.mcu_ver;
    
    payload_len = cpu_to_be16(header->len);
    if(payload_len > sizeof(nc_audio_get_mcu_ver_t)) {
        QM_LOGE("1", "mcu_get_ver_unpack payload_len:%d is too long", payload_len);
        return -QM_EINVAL;
    }
    memcpy(mcu_ver, header->payload, payload_len - 1);

    if(!mcu_ver->type){
        comm_base_cmd_send(NC_AUDIO_SUB_CMD_GET_MCU_VER, CONFIG_NC_AUDIO_SEND_COUNT, CONFIG_NC_AUDIO_SEND_TIMEOUT_MS, (void *)&send_data);
    }else{
        
        spk_cfg.bit = mic_cfg.bit = g_comm_ctx->bits;
        spk_cfg.type = mic_cfg.type = g_comm_ctx->fmt_type;
        spk_cfg.rate = mic_cfg.rate = cpu_to_be32(g_comm_ctx->rate);
        spk_cfg.size = mic_cfg.size  = cpu_to_be32(CONFIG_NC_AUDIO_RECV_MAX_SIZE);

        mic_cfg.channel = g_comm_ctx->channel;
        mic_cfg.mic_gain = g_comm_ctx->mic_gain;
        send_data.playload = (uint8_t *)&mic_cfg;
        send_data.playload_len = sizeof(nc_audio_mic_cfg_t);
        comm_base_cmd_send(NC_AUDIO_SUB_CMD_MIC_CFG, CONFIG_NC_AUDIO_SEND_COUNT, CONFIG_NC_AUDIO_SEND_TIMEOUT_MS, (void *)&send_data);

        spk_cfg.spk_gain = g_comm_ctx->spk_gain;

        send_data.playload = (uint8_t *)&spk_cfg;
        send_data.playload_len = sizeof(nc_audio_spk_cfg_t);
        comm_base_cmd_send(NC_AUDIO_SUB_CMD_SPK_CFG, CONFIG_NC_AUDIO_SEND_COUNT, CONFIG_NC_AUDIO_SEND_TIMEOUT_MS, (void *)&send_data);
        

        nc_audio_cfg(NC_AUDIO_CFG_TYPE_VAD_ONOFF, g_comm_ctx->vad_timeout);
        
    }
    
    if(g_comm_ctx->event_notify){
        g_comm_ctx->event_notify(NC_AUDIO_EVENT_TYPE_GET_MCU_VER, &event);
    }
    
    return QM_EOK;
}

static int mcu_get_ver_pack(uint8_t *data, int *len, void *arg)
{
    nc_audio_fram_check_t *chk = NULL;
    nc_audio_send_data_t *send_data = (nc_audio_send_data_t *)arg;

    if(data == NULL){  
        goto __exit;
    }

    frame_pack(NC_AUDIO_SUB_CMD_GET_MCU_VER, (nc_audio_frame_header_t *)data, send_data->playload, send_data->playload_len);

    chk = (nc_audio_fram_check_t *)(data + NC_AUDIO_FRAME_MIN_LEN + send_data->playload_len - 1); 
    chk->add8 =  frame_chksum(data, *len - 1);

__exit:

    *len = send_data->playload_len + NC_AUDIO_FRAME_MIN_LEN;

    return QM_EOK;
}

static int mic_cfg_unpack(uint8_t *data, int len)
{

    nc_audio_event_param_t event = {0};
    nc_audio_code_t *mic_cfg_result = NULL;
    nc_audio_frame_header_t *header = (nc_audio_frame_header_t *)data;

    mic_cfg_result = &event.mic_cfg_result;
    mic_cfg_result->code = header->payload[0];

    if(g_comm_ctx->event_notify){
        g_comm_ctx->event_notify(NC_AUDIO_EVENT_TYPE_MIC_CFG, &event);
    }
    
    return QM_EOK;
}

static int mic_cfg_pack(uint8_t *data, int *len, void *arg)
{
    nc_audio_fram_check_t *chk = NULL;
    nc_audio_send_data_t *send_data = (nc_audio_send_data_t *)arg;

    if(data == NULL){  
        goto __exit;
    }

    frame_pack(NC_AUDIO_SUB_CMD_MIC_CFG, (nc_audio_frame_header_t *)data, send_data->playload, send_data->playload_len);
    
    chk = (nc_audio_fram_check_t *)(data + NC_AUDIO_FRAME_HEAD_LEN + send_data->playload_len); 
    
    chk->add8 =  frame_chksum(data, *len - 1);

__exit:

    *len = send_data->playload_len + NC_AUDIO_FRAME_MIN_LEN;

    return QM_EOK;
}

static int spk_cfg_unpack(uint8_t *data, int len)
{
    nc_audio_event_param_t event = {0};
    nc_audio_code_t *spk_cfg_result = NULL;
    nc_audio_frame_header_t *header = (nc_audio_frame_header_t *)data;

    spk_cfg_result = &event.spk_cfg_result;
    

    spk_cfg_result->code = header->payload[0];

    if(g_comm_ctx->event_notify){
        g_comm_ctx->event_notify(NC_AUDIO_EVENT_TYPE_SPK_CFG, &event);
    }
    
    return QM_EOK;
}

static int spk_cfg_pack(uint8_t *data, int *len, void *arg)
{
    nc_audio_fram_check_t *chk = NULL;
    nc_audio_send_data_t *send_data = (nc_audio_send_data_t *)arg;

    if(data == NULL){  
        goto __exit;
    }

    frame_pack(NC_AUDIO_SUB_CMD_SPK_CFG, (nc_audio_frame_header_t *)data, send_data->playload, send_data->playload_len);
    
    chk = (nc_audio_fram_check_t *)(data + NC_AUDIO_FRAME_HEAD_LEN + send_data->playload_len); 
    
    chk->add8 =  frame_chksum(data, *len - 1);

__exit:

    *len = send_data->playload_len + NC_AUDIO_FRAME_MIN_LEN;

    return QM_EOK;
}

static int mic_data_unpack(uint8_t *data, int len)
{
    uint8_t code = 0;
    uint16_t payload_len = 0;
    nc_audio_event_param_t event = {0};
    nc_audio_send_data_t send_data = {0};
    nc_audio_mic_data_t *mic_data = NULL;
    nc_audio_frame_header_t *header = (nc_audio_frame_header_t *)data;

    mic_data = &event.mic_data;
    
    payload_len = cpu_to_be16(header->len) - 2;
    mic_data->status = header->payload[0];

    code = 0;
    send_data.playload = &code;
    send_data.playload_len = sizeof(uint8_t);
#if 0
    comm_base_cmd_send(NC_AUDIO_SUB_CMD_MIC_DATA, CONFIG_NC_AUDIO_SEND_COUNT, CONFIG_NC_AUDIO_SEND_TIMEOUT_MS, (void *)&send_data);
#endif
    if(mic_data->status == NC_AUDIO_VAD_DATA && payload_len ){
        mic_data->payload_len = payload_len;
        mic_data->payload = &header->payload[1];
    }

    if(g_comm_ctx->event_notify){
        g_comm_ctx->event_notify(NC_AUDIO_EVENT_TYPE_MIC_DATA_GET, &event);
    }
    
    return QM_EOK;
}

static int mic_data_pack(uint8_t *data, int *len, void *arg)
{
    nc_audio_fram_check_t *chk = NULL;
    nc_audio_send_data_t *send_data = (nc_audio_send_data_t *)arg;

    if(data == NULL){  
        goto __exit;
    }

    frame_pack(NC_AUDIO_SUB_CMD_MIC_DATA, (nc_audio_frame_header_t *)data, send_data->playload, send_data->playload_len);
    
    chk = (nc_audio_fram_check_t *)(data + NC_AUDIO_FRAME_HEAD_LEN + send_data->playload_len); 
    
    chk->add8 =  frame_chksum(data, *len - 1);

__exit:

    *len = send_data->playload_len + NC_AUDIO_FRAME_MIN_LEN;

    return QM_EOK;
}

static int spk_play_unpack(uint8_t *data, int len)
{
    nc_audio_event_param_t event = {0};
    nc_audio_code_t *spk_play_result = NULL;
    nc_audio_frame_header_t *header = (nc_audio_frame_header_t *)data;

    spk_play_result = &event.spk_play_ack;
    
    spk_play_result->code = header->payload[0];
    spk_play_result->msg_id = (
        (header->payload[1] << 24)| (header->payload[2] << 16) |
        (header->payload[3] << 8) | (header->payload[4]) );

    if(g_comm_ctx->event_notify){
        g_comm_ctx->event_notify(NC_AUDIO_EVENT_TYPE_SPK_PLAY_ACK, &event);
    }
    
    return QM_EOK;
}

static int spk_play_pack(uint8_t *data, int *len, void *arg)
{
    nc_audio_fram_check_t *chk = NULL;
    nc_audio_send_data_t *send_data = (nc_audio_send_data_t *)arg;

    if(data == NULL){
        goto __exit;
    }

    frame_pack(NC_AUDIO_SUB_CMD_SPK_PLAY, (nc_audio_frame_header_t *)data, send_data->playload, send_data->playload_len);
    
    chk = (nc_audio_fram_check_t *)(data + NC_AUDIO_FRAME_HEAD_LEN + send_data->playload_len); 
    
    chk->add8 =  frame_chksum(data, *len - 1);

__exit:

    *len = send_data->playload_len + NC_AUDIO_FRAME_MIN_LEN;

    return QM_EOK;
}

static int wakeup_unpack(uint8_t *data, int len)
{
#if 0
    uint16_t payload_len = 0;
    nc_audio_event_param_t event = {0};
    nc_audio_code_t *spk_play_result = NULL;
    nc_audio_frame_header_t *header = (nc_audio_frame_header_t *)data;
#endif

    if(g_comm_ctx->event_notify){
        g_comm_ctx->event_notify(NC_AUDIO_EVENT_TYPE_WAKEUP, NULL);
    }
    

    return QM_EOK;
}

static int wakeup_pack(uint8_t *data, int *len, void *arg)
{
    return QM_EOK;
}

static int sleep_unpack(uint8_t *data, int len)
{
#if 0
    uint16_t payload_len = 0;
    nc_audio_event_param_t event = {0};
    nc_audio_code_t *spk_play_result = NULL;
    nc_audio_frame_header_t *header = (nc_audio_frame_header_t *)data;
#endif

    if(g_comm_ctx->event_notify){
        g_comm_ctx->event_notify(NC_AUDIO_EVENT_TYPE_SLEEP, NULL);
    }
    
    return QM_EOK;
}

static int sleep_pack(uint8_t *data, int *len, void *arg)
{

    return QM_EOK;
}

static int cfg_unpack(uint8_t *data, int len)
{
    return QM_EOK;
}

static int cfg_pack(uint8_t *data, int *len, void *arg)
{
    nc_audio_fram_check_t *chk = NULL;
    nc_audio_send_data_t *send_data = (nc_audio_send_data_t *)arg;

    if(data == NULL){  
        goto __exit;
    }

    frame_pack(NC_AUDIO_SUB_CMD_AUDIO_CFG, (nc_audio_frame_header_t *)data, send_data->playload, send_data->playload_len);
    
    chk = (nc_audio_fram_check_t *)(data + NC_AUDIO_FRAME_HEAD_LEN + send_data->playload_len); 
    
    chk->add8 =  frame_chksum(data, *len - 1);

__exit:

    *len = send_data->playload_len + NC_AUDIO_FRAME_MIN_LEN;

    return QM_EOK;
}

static int record_func_unpack(uint8_t *data, int len)
{

    return QM_EOK;
}

static int record_func_pack(uint8_t *data, int *len, void *arg)
{
    nc_audio_fram_check_t *chk = NULL;
    nc_audio_send_data_t *send_data = (nc_audio_send_data_t *)arg;

    if(data == NULL){  
        goto __exit;
    }

    frame_pack(NC_AUDIO_SUB_CMD_RECORD_FUNC, (nc_audio_frame_header_t *)data, send_data->playload, send_data->playload_len);
    
    chk = (nc_audio_fram_check_t *)(data + NC_AUDIO_FRAME_HEAD_LEN + send_data->playload_len); 
    
    chk->add8 =  frame_chksum(data, *len - 1);

__exit:

    *len = send_data->playload_len + NC_AUDIO_FRAME_MIN_LEN;

    return QM_EOK;
}

static int spk_reply_unpack(uint8_t *data, int len)
{
    nc_audio_event_param_t event = {0};
    nc_audio_spk_state_t *spk_state = NULL;
    nc_audio_frame_header_t *header = (nc_audio_frame_header_t *)data;

    spk_state = &event.spk_state;
    
    if(cpu_to_be16(header->len) != 5) {
        return QM_EINVAL;
    }

    spk_state->status = (
        (header->payload[0] << 24)| (header->payload[1] << 16) |
        (header->payload[2] << 8) | (header->payload[3]) );


    if(g_comm_ctx->event_notify){
        g_comm_ctx->event_notify(NC_AUDIO_EVENT_TYPE_SPK_REPLY, &event);
    }

    return QM_EOK;
}

static int spk_reply_pack(uint8_t *data, int *len, void *arg)
{
    return QM_EOK;
}

int mcu_set_mic_gain(uint8_t gain) 
{
    nc_audio_mic_cfg_t mic_cfg = {0};
    nc_audio_send_data_t send_data = {0};
    mic_cfg.bit = g_comm_ctx->bits;
    mic_cfg.type = g_comm_ctx->fmt_type;
    mic_cfg.rate = cpu_to_be32(g_comm_ctx->rate);
    mic_cfg.size  = cpu_to_be32(CONFIG_NC_AUDIO_RECV_MAX_SIZE);

    mic_cfg.channel = g_comm_ctx->channel;
    mic_cfg.mic_gain = gain;
    send_data.playload = (uint8_t *)&mic_cfg;
    send_data.playload_len = sizeof(nc_audio_mic_cfg_t);
    comm_base_cmd_send(NC_AUDIO_SUB_CMD_MIC_CFG, CONFIG_NC_AUDIO_SEND_COUNT, CONFIG_NC_AUDIO_SEND_TIMEOUT_MS, (void *)&send_data);
}
