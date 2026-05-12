#include "qm.h"
#include "qm_gpio.h"
#include "qm_platform.h"
#include "yopen_gpio.h"



typedef struct{
    yopen_GpioDir dir;
    yopen_PullMode pull;
}gpio_config_t;

typedef struct{
    yopen_GpioNum pin_id;
    uint8_t func_id;
    gpio_config_t *config;
}gpio_list_info_t;

static gpio_list_info_t g_gpio_info[200] = 
{
    [QM_GPIO_PIN_82] = {GPIO_0, 0, NULL},   
    [QM_GPIO_PIN_22] = {GPIO_1, 0, NULL},   
    [QM_GPIO_PIN_23] = {GPIO_2, 0, NULL},   
    [QM_GPIO_PIN_54] = {GPIO_3, 0, NULL},   
    [QM_GPIO_PIN_80] = {GPIO_4, 0, NULL},   
    [QM_GPIO_PIN_81] = {GPIO_5, 0, NULL},   
    [QM_GPIO_PIN_55] = {GPIO_6, 0, NULL},   
    [QM_GPIO_PIN_56] = {GPIO_7, 0, NULL},   
    [QM_GPIO_PIN_66] = {GPIO_8, 0, NULL},   
    [QM_GPIO_PIN_67] = {GPIO_9, 0, NULL},   
    [QM_GPIO_PIN_28] = {GPIO_10, 0, NULL},  
    [QM_GPIO_PIN_29] = {GPIO_11, 0, NULL},  
    [QM_GPIO_PIN_64] = {GPIO_12, 0, NULL},  
    [QM_GPIO_PIN_63] = {GPIO_13, 0, NULL},  
    [QM_GPIO_PIN_62] = {GPIO_14, 0, NULL},  
    [QM_GPIO_PIN_49] = {GPIO_15, 0, NULL},  
    [QM_GPIO_PIN_38] = {GPIO_16, 0, NULL},  
    [QM_GPIO_PIN_39] = {GPIO_17, 0, NULL},  
    [QM_GPIO_PIN_17] = {GPIO_18, 0, NULL},  
    [QM_GPIO_PIN_18] = {GPIO_19, 0, NULL},  
    [QM_GPIO_PIN_5] = {GPIO_20, 0, NULL},   
    [QM_GPIO_PIN_6] = {GPIO_21, 0, NULL},   
    [QM_GPIO_PIN_19] = {GPIO_22, 0, NULL},  
    [QM_GPIO_PIN_100] = {GPIO_23, 0, NULL}, 
    [QM_GPIO_PIN_101] = {GPIO_24, 0, NULL}, 
    [QM_GPIO_PIN_16] = {GPIO_25, 0, NULL},  
    [QM_GPIO_PIN_25] = {GPIO_26, 0, NULL},  
    [QM_GPIO_PIN_20] = {GPIO_27, 0, NULL},  
    // GPIO28 内部占用
    [QM_GPIO_PIN_30] = {GPIO_29, 0, NULL},  
    [QM_GPIO_PIN_31] = {GPIO_30, 0, NULL},  
    [QM_GPIO_PIN_32] = {GPIO_31, 0, NULL},  
    [QM_GPIO_PIN_33] = {GPIO_32, 0, NULL},  
    [QM_GPIO_PIN_26] = {GPIO_33, 0, NULL},  
    [QM_GPIO_PIN_53] = {GPIO_34, 0, NULL},  
    [QM_GPIO_PIN_52] = {GPIO_35, 0, NULL},  
    [QM_GPIO_PIN_78] = {GPIO_36, 0, NULL},  
    [QM_GPIO_PIN_50] = {GPIO_37, 0, NULL},  
    [QM_GPIO_PIN_51] = {GPIO_38, 0, NULL},  
};




int32_t qm_gpio_init(qm_gpio_dev_t *gpio)
{
    int ret = 0;
    static int init = 1;
    yopen_LvlMode lvl_mode = LVL_LOW;
    gpio_list_info_t *gpio_info = NULL;

    if(gpio == NULL){
        return -QM_EINVAL;
    }

    if(gpio->port >= QM_ARRAY_SIZE(g_gpio_info)){
        return -QM_EINVAL;
    }
    gpio_info = &g_gpio_info[gpio->port];

    gpio_config_t *gpio_config = (gpio_config_t *)qm_malloc(sizeof(gpio_config_t));
    if(gpio_config == NULL){
        return -QM_ENOMEM;
    }
    memset(gpio_config, 0, sizeof(gpio_config_t));

    yopen_gpio_set_voltage(Vol_1_80V);

    yopen_gpio_deinit(gpio_info->pin_id);
	
    if(init){
        init = 0;
        //使能AON POWER AON与普通GPIO电压不同，需要单独使能电源域
	    yopen_aon_power_on();	
    }

    switch (gpio->config.mode)
    {
        case QM_GPIO_MODE_INPUT:
            gpio_config->dir = GPIO_INPUT;
            break;
        case QM_GPIO_MODE_OUTPUT:
            gpio_config->dir = GPIO_OUTPUT;
            break;
        default:
            ret = -QM_EIO;
            break;
    }

    switch (gpio->config.pull_en)
    {
        case QM_GPIO_PULLUP_ONLY:
            lvl_mode = LVL_HIGH;
            gpio_config->pull = FORCE_PULL_UP;
            break;
        case QM_GPIO_PULLDOWN_ONLY:
            lvl_mode = LVL_LOW;
            gpio_config->pull = FORCE_PULL_DOWN;
            break;
        case QM_GPIO_FLOATING:
            gpio_config->pull = FORCE_PULL_NONE;
            break;
        default:
            ret = -QM_EIO;
            break;
    }

    if(ret != QM_EOK){
        qm_free(gpio_config);
        return ret;
    }

#if 0
    ret = yopen_pin_set_func(gpio->port, gpio_info->func_id);
    if(ret != YOPEN_GPIO_SUCCESS){
        qm_free(gpio_config);
        return -QM_EIO;
    }

    ret = yopen_gpio_init(gpio_info->pin_id, gpio_config->dir, gpio_config->pull, lvl_mode);
    if(ret != YOPEN_GPIO_SUCCESS){
        qm_free(gpio_config);
        return -QM_EIO;
    }
#else
    yopen_pin_set_func(gpio->port, gpio_info->func_id);
    yopen_gpio_init(gpio_info->pin_id, gpio_config->dir, gpio_config->pull, lvl_mode);
#endif

    gpio_info->config = gpio_config;

    gpio->priv = gpio_info;

    return QM_EOK;
}

int32_t qm_gpio_set_level(qm_gpio_dev_t *gpio, uint8_t level)
{
    int ret = QM_EOK;
    yopen_LvlMode lvl_mode = LVL_LOW;
    if (gpio == NULL || gpio->priv == NULL){
        return -QM_EINVAL;
    }
    gpio_list_info_t *gpio_info = (gpio_list_info_t *)gpio->priv;

    if(level){
        lvl_mode = LVL_HIGH;
    }else{
        lvl_mode = LVL_LOW;
    }
    ret = yopen_gpio_set_level(gpio_info->pin_id, lvl_mode);

    return QM_EOK;
}

uint8_t qm_gpio_get_level(qm_gpio_dev_t *gpio)
{
    uint8_t level = 0;
    yopen_LvlMode lvl_mode = LVL_LOW;
    gpio_list_info_t *gpio_info = (gpio_list_info_t *)gpio->priv;
    if (gpio == NULL || gpio->priv == NULL){
        return -QM_EINVAL;
    }

    yopen_gpio_get_level(gpio_info->pin_id, &lvl_mode);

    if(lvl_mode == LVL_HIGH){
        level = 1;
    }else{
        level = 0;
    }

    return level;
}

int32_t qm_gpio_enable_irq(qm_gpio_dev_t *gpio, qm_gpio_intr_type_t intr_type, qm_gpio_irq_handler_t handler, void *arg)
{
    yopen_EdgeMode edge = EDGE_RISING;
    yopen_TriggerMode trigger = EDGE_TRIGGER;
    gpio_list_info_t *gpio_info = (gpio_list_info_t *)gpio->priv;
    if (gpio == NULL || gpio->priv == NULL){
        return -QM_EINVAL;
    }
    gpio_config_t *gpio_config = (gpio_config_t *)gpio_info->config;

    switch(intr_type)
    {
        case QM_GPIO_INTR_ANYEDGE: //下降沿
            edge = EDGE_BOTH;
            trigger = EDGE_TRIGGER;
            break;
        case QM_GPIO_INTR_POSEDGE:    //上升沿
            edge = EDGE_RISING;
            trigger = EDGE_TRIGGER;
            break;
        case QM_GPIO_INTR_NEGEDGE: //下降沿
            edge = EDGE_FALLING;
            trigger = EDGE_TRIGGER;
            break;
        case QM_GPIO_INTR_LOW_LEVEL: //低电平
            edge = EDGE_RISING;
            trigger = LEVEL_TRIGGER;
            break;
        case QM_GPIO_INTR_HIGH_LEVEL: //高电平
            edge = EDGE_FALLING;
            trigger = LEVEL_TRIGGER;
            break;

        default:
            
            break;
    }

    
    /*
    * MAIN_RI 引脚的中断回调中，获取中断引脚电平，并发送消息到消息队列处理任务中
    * 
    * Notes: 中断回调中不可执行耗时较长的操作，不可调用阻塞性接口，否则会引起系统异常。
    */
    yopen_int_register(gpio_info->pin_id, trigger, DEBOUNCE_DIS, edge, gpio_config->pull, handler, arg);
    
    yopen_int_enable(gpio_info->pin_id);
    return QM_EOK;
}

int32_t qm_gpio_disable_irq(qm_gpio_dev_t *gpio)
{
    gpio_list_info_t *gpio_info = (gpio_list_info_t *)gpio->priv;
    if (gpio == NULL || gpio->priv == NULL){
        return -QM_EINVAL;
    }
    gpio_config_t *gpio_config = (gpio_config_t *)gpio_info->config;

    yopen_int_disable(gpio_info->pin_id);

    return QM_EOK;
}

int32_t qm_gpio_clear_irq(qm_gpio_dev_t *gpio)
{
    return QM_EOK;
}

int32_t qm_gpio_deinit(qm_gpio_dev_t *gpio)
{
    return QM_EOK;
}