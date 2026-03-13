#ifndef _NC_AUDIO_H_
#define _NC_AUDIO_H_

#include "qm.h"

#ifdef __cplusplus
extern "C" {
#endif


#ifndef CONFIG_NC_AUDIO_RECV_MAX_SIZE  
#define CONFIG_NC_AUDIO_RECV_MAX_SIZE        (512)
#endif

/// 数据接收超时(从帧头开始计时)
#ifndef CONFIG_NC_AUDIO_RECV_TIMEOUT_MS  
#define CONFIG_NC_AUDIO_RECV_TIMEOUT_MS      (2000)
#endif

/// 数据接收超时(从帧头开始计时)
#ifndef CONFIG_NC_AUDIO_RECV_VER_MAX_LEN  
#define CONFIG_NC_AUDIO_RECV_VER_MAX_LEN      (20)
#endif

/// 数据发送次数
#ifndef CONFIG_NC_AUDIO_SEND_COUNT 
#define CONFIG_NC_AUDIO_SEND_COUNT            (1)
#endif

/// 数据接收超时(从帧头开始计时)
#ifndef CONFIG_NC_AUDIO_SEND_TIMEOUT_MS  
#define CONFIG_NC_AUDIO_SEND_TIMEOUT_MS      (100)
#endif

/*
 *   ----------------------------------------------------------------
 *  |  FrameHeader |  VER   |  CMD    | LEN    |  DATA  |  ADD8 |
 *   ----------------------------------------------------------------
 *  |     2Bytes   |  1Byte |  1Byte  | 2Bytes | NBytes  | 1Byte |
 */

#define NC_AUDIO_FRAME_HEADER_1  (0x55)
#define NC_AUDIO_FRAME_HEADER_2  (0xAA)
#define NC_AUDIO_FRAME_VER       (0x03)
#define NC_AUDIO_FRAME_VER_1     (0x00)
#define NC_AUDIO_FRAME_CMD       (0x92)

#define NC_AUDIO_FRAM_PORT_LEN   (3)    /*HEAD(2byte) + VER(1byte) */    
#define NC_AUDIO_FRAME_HEAD_LEN  (7)    /*HEAD(2byte) + VER(1byte)  + CMD(1byte) + LEN(2byte) + SUB_CMD(1byte)*/
#define NC_AUDIO_FRAME_MIN_LEN   (8)    /*HEAD(2byte) + VER(1byte) + CMD(1byte)+ LEN(2byte) + SUB_CMD(1byte) + ADDR8(1byte)*/


#define NC_AUDIO_FRAME_SUB_CMD_ADDR  (6)    /*HEAD(2byte) + VER(1byte) + CMD(1byte)+ LEN(2byte) + SUB_CMD(1byte) + ADDR8(1byte)*/


//子命令
typedef enum
{
    NC_AUDIO_SUB_CMD_NONE                   = 0x00,
    NC_AUDIO_SUB_CMD_GET_MCU_VER            = 0x01, //MCU版本查询
    NC_AUDIO_SUB_CMD_MIC_CFG                = 0x02, //离线语⾳MIC配置
    NC_AUDIO_SUB_CMD_SPK_CFG                = 0x03, //离线语⾳SPK配置
    NC_AUDIO_SUB_CMD_WAKE_IO_CFG            = 0x04, //唤醒引脚设置
    NC_AUDIO_SUB_CMD_MIC_DATA               = 0x05, //MIC数据传输
    NC_AUDIO_SUB_CMD_SPK_PLAY               = 0x06, //SPK播放
    NC_AUDIO_SUB_CMD_AUDIO_CFG              = 0x07, //离线语⾳配置命令
    NC_AUDIO_SUB_CMD_AUDIO_WAKEUP           = 0x08, //离线语⾳唤醒事件
    NC_AUDIO_SUB_CMD_AUDIO_SLEEP            = 0x09, //离线语⾳休眠事件
    NC_AUDIO_SUB_CMD_GET_STATUS             = 0x0A, //离线语⾳芯⽚状态查询
    NC_AUDIO_SUB_CMD_RECORD_TEST            = 0x0B, //离线语⾳录⾳播⾳测试
    NC_AUDIO_SUB_CMD_SKP_REPLY              = 0x12, //语音模组SPK播放事件上报
    NC_AUDIO_SUB_CMD_RECORD_FUNC            = 0xF5, //录音模式开关
    NC_AUDIO_SUB_CMD_PLAY_STATE_NOTIFY      = 0xF6, //播放状态通知
}nc_audio_frame_sub_cmd_t;


typedef enum
{
    NC_AUDIO_PARSE_STATE_HEAD_1 = 1,
    NC_AUDIO_PARSE_STATE_HEAD_2,
    NC_AUDIO_PARSE_STATE_VER,
    NC_AUDIO_PARSE_STATE_CMD,
    NC_AUDIO_PARSE_STATE_LEN,
    NC_AUDIO_PARSE_STATE_DATA,
    NC_AUDIO_PARSE_STATE_MAX,
}nc_audio_parse_state_t;


typedef enum{
    NC_AUDIO_EVENT_TYPE_WAKEUP,
    NC_AUDIO_EVENT_TYPE_SLEEP,
    NC_AUDIO_EVENT_TYPE_GET_MCU_VER,
    NC_AUDIO_EVENT_TYPE_MIC_CFG,
    NC_AUDIO_EVENT_TYPE_SPK_CFG,
    NC_AUDIO_EVENT_TYPE_MIC_DATA_GET,
    NC_AUDIO_EVENT_TYPE_SPK_PLAY_ACK,
    NC_AUDIO_EVENT_TYPE_SPK_REPLY,
    NC_AUDIO_EVENT_MAX,
}nc_audio_event_type_t;

typedef enum {
    NC_AUDIO_FMT_TYPE_PCM = 0,
    NC_AUDIO_FMT_TYPE_SPEEX,
    NC_AUDIO_FMT_TYPE_OPUS,
    NC_AUDIO_FMT_TYPE_MP3,
} nc_audio_fmt_type_t;    


#pragma pack(1)

typedef union 
{
    uint8_t add8;
}nc_audio_fram_check_t;

typedef struct 
{   
    uint8_t head_1;
    uint8_t head_2;
    uint8_t ver;
    uint8_t cmd;
    uint16_t len;
    uint8_t sub_cmd;
    uint8_t payload[1]; 
}nc_audio_frame_header_t;

typedef struct 
{   
    uint32_t rate;
    uint8_t bit; 
    uint8_t channel; 
    uint8_t mic_gain;
    uint8_t type; //数据格式：0 (PCM)；1 (speex)；2 (opus)
    uint32_t size;
}nc_audio_mic_cfg_t;

typedef struct 
{   
    uint32_t rate;
    uint8_t bit; 
    uint8_t spk_gain;
    uint8_t type; //数据格式：0 (PCM)；1 (speex)；2 (opus)
    uint32_t size;
}nc_audio_spk_cfg_t;

typedef struct 
{   
   uint8_t type; //0 (开始播放)；1 (播放数据)；2 (停⽌播放)
   uint32_t msg_id; //包序号（⾃增，⾮每次从0开始）
}nc_audio_spk_play_header_t;

typedef struct 
{   
   uint8_t type; //0 (开始播放)；1 (播放数据)；2 (停⽌播放)
   uint32_t msg_id; //包序号（⾃增，⾮每次从0开始）
   uint8_t playload[1];
}nc_audio_spk_play_t;

typedef struct 
{   
   uint8_t type; //
   uint8_t value; //
}nc_audio_cfg_t;

typedef struct 
{   
   uint8_t type; //
   int value; //
}nc_audio_ext_cfg_t;

typedef struct 
{   
   uint8_t on_off; // 录音开关
   uint8_t ns_level; //降噪等级
}nc_audio_record_func_t;


typedef struct 
{
   uint8_t type; //0：MCU上电⾸次发送；1：收到模组查询返回
   uint8_t status; //SPK流控IO（0xFF表⽰不使⽤流控），⽤于SPK播放（0x06）控制
   uint8_t level; //有效电平：0（低电平有效，IO拉低可发送）；1 (⾼电平有效，IO拉⾼可发送)
   char    str_ver[CONFIG_NC_AUDIO_RECV_VER_MAX_LEN]; //版本号字符串（如"1.0.0"，最⼤10字节，格式x.x.x，x为0-99的⼗进制数）
}nc_audio_get_mcu_ver_t;

#pragma pack()

typedef enum {
    NC_AUDIO_SPK_PLAY_TYPE_START = 0,
    NC_AUDIO_SPK_PLAY_TYPE_DATA,
    NC_AUDIO_SPK_PLAY_TYPE_STOP,
} nc_audio_spk_play_type_t;  

typedef enum {
    NC_AUDIO_SPK_REPLY_TYPE_START = 1,
    NC_AUDIO_SPK_REPLY_TYPE_EMPTY,
    NC_AUDIO_SPK_REPLY_TYPE_STOP,
} nc_audio_spk_reply_type_t;  

typedef enum 
{
    NC_AUDIO_VAD_START = 0,
    NC_AUDIO_VAD_DATA,
    NC_AUDIO_VAD_STOP,
}nc_audio_vad_status_t;

typedef enum {
    NC_AUDIO_CFG_TYPE_CHANNEL = 0,
    NC_AUDIO_CFG_TYPE_MIC_ONOFF,
    NC_AUDIO_CFG_TYPE_SPK_ONOFF,
    NC_AUDIO_CFG_TYPE_WAKEUP_ONOFF,
    NC_AUDIO_CFG_TYPE_VAD_ONOFF,
    NC_AUDIO_CFG_TYPE_WAKEUP_ONTIFY_ONOFF,
    NC_AUDIO_CFG_TYPE_TIMEOUT_ONTIFY_ONOFF,

    NC_AUDIO_CFG_TYPE_SLIENT_TIMEOUT = 7,
    NC_AUDIO_CFG_TYPE_MAX_RECORD_TIME = 8,
    NC_AUDIO_CFG_TYPE_VAD_SENSITIVITY = 9,
    NC_AUDIO_CFG_TYPE_NOISE_REDUCTION = 10,


} nc_audio_cfg_type_t;  

typedef struct 
{
   uint32_t msg_id;
   uint8_t code; //执⾏结果：0x00 (成功)；0x01 (失败
}nc_audio_code_t;

typedef struct 
{
   nc_audio_vad_status_t status; //0 (vad_start)；1 (vad_data)；2 (vad_end)
   uint16_t payload_len;
   uint8_t *payload;
}nc_audio_mic_data_t;

typedef struct 
{
   nc_audio_spk_reply_type_t status; //1 (spk_start)；2 (spk_empty)；3 (spk_stop)
}nc_audio_spk_state_t;

typedef union 
{
    nc_audio_get_mcu_ver_t mcu_ver;
    nc_audio_code_t mic_cfg_result;
    nc_audio_code_t spk_cfg_result;
    nc_audio_code_t spk_play_ack;
    nc_audio_mic_data_t mic_data;
    nc_audio_spk_state_t spk_state;
}nc_audio_event_param_t;

typedef int (*nc_audio_send_t)(uint8_t *data, int len);

typedef int (*nc_audio_event_t)(nc_audio_event_type_t event_type, nc_audio_event_param_t *p_event);

typedef struct
{
    nc_audio_send_t send;                  //当调用下发接口时，触发该回调函数，用户使用实际的接口进行数据下发，如qm_write
    nc_audio_event_t notify;               //事件回调函数
    nc_audio_fmt_type_t           fmt_type;          /*!< 音频格式*/
    uint16_t                      rate;              /*!< 音频采样率*/
    uint16_t                      bits;              /*!< 音频采样位数*/
    uint16_t                      channel;           /*!< 音频声道数*/
    uint8_t                       mic_gain;          /*!< mic增益*/
    uint8_t                       spk_gain;          /*!< spk增益*/
    uint8_t                       vad_tiemout;      /*!< vad timeout*/
}nc_audio_param_t;

/**
 * @brief 初始化模块并设置默认参数
 *
 * @param[in] param 初始化参数，详见 @ref nc_audio_param_t
 *
 * @return int32_t
 * @retval <QM_EOK 执行失败
 * @retval >=QM_EOK 执行成功
 */
int32_t nc_audio_init(nc_audio_param_t *param);


/**
 * @brief 事件重新注册
 *
 * @param[in] param 初始化参数，详见 @ref nc_audio_param_t
 *
 * @return int32_t
 * @retval <QM_EOK 执行失败
 * @retval >=QM_EOK 执行成功
 */
int32_t nc_audio_event_register(nc_audio_event_t event_notify);

/**
 * @brief 传输一条报文到组件包，内部会malloc数据
 *
 * @param[in] data 数据句柄
 * @param[in] len 数据长度
 * 
 *
 * @return int32_t
 * @retval <QM_EOK 执行失败
 * @retval >=QM_EOK 执行成功
 */
int32_t nc_audio_recv_push(uint8_t *data, int len);

/**
 * @brief 音频播报
 *
 * @param[in] data 数据句柄
 * @param[in] len 数据长度
 * 
 *
 * @return int32_t
 * @retval <QM_EOK 执行失败
 * @retval >=QM_EOK 执行成功
 */
int32_t nc_audio_spk_play(nc_audio_spk_play_header_t *play, uint8_t *payload, int payload_len);
int32_t nc_play_spk_direct_play(nc_audio_spk_play_header_t *play, uint8_t *payload, int payload_len);


/**
 * @brief 音频配置
 *
 * @param[in] value 音量增益: 0~100
 * 
 *
 * @return int32_t
 * @retval <QM_EOK 执行失败
 * @retval >=QM_EOK 执行成功
 */
int32_t nc_audio_set_spk_volume(uint8_t value);

/**
 * @brief 音频配置
 *
 * @param[in] data 数据句柄
 * @param[in] len 数据长度
 * 
 *
 * @return int32_t
 * @retval <QM_EOK 执行失败
 * @retval >=QM_EOK 执行成功
 */
int32_t nc_audio_cfg(nc_audio_cfg_type_t type, int value);


/**
 * @brief 录音开关
 *
 * @param[in] data 数据句柄
 * @param[in] len 数据长度
 * 
 *
 * @return int32_t
 * @retval <QM_EOK 执行失败
 * @retval >=QM_EOK 执行成功
 */
int32_t nc_audio_record_func(nc_audio_record_func_t func);

/**
 * @brief 设置MIC增益
 *
 * @param[in] gain 数据长度
 * 
 *
 * @return int
 * @retval <QM_EOK 执行失败
 * @retval >=QM_EOK 执行成功
 */
int mcu_set_mic_gain(uint8_t gain);
#ifdef __cplusplus
}
#endif

#endif
