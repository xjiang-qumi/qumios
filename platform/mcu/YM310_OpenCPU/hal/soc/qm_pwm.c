#include "qm_pwm.h"
#include "yopen_pwm.h"

#define QM_PWM_DBG(...) //QM_LOGD("PWM", __VA_ARGS__)

typedef struct {
    uint8_t pin_alt_func;  //管脚复用
} qm_pwm_priv_t;

/**
 * @brief Convert port number to yopen_pwm_sel
 * 
 * @param port PWM port number (0-4)
 * @return yopen_pwm_sel Corresponding yopen PWM channel
 */
static inline yopen_pwm_sel port_to_channel(uint8_t port)
{
    static const yopen_pwm_sel channel_map[] = {
        YOPEN_PWM_0, YOPEN_PWM_1, YOPEN_PWM_2, YOPEN_PWM_3, YOPEN_PWM_4
    };
    if (port >= sizeof(channel_map) / sizeof(channel_map[0])) {
        return PWM_MAX;
    }
    return channel_map[port];
}

/**
 * @brief Convert qm_pwm_config_t to yopen_pwm_cfg_s
 * 
 * @param pwm PWM device
 * @param cfg Output yopen PWM config
 */
static inline void config_to_yopen(const qm_pwm_dev_t *pwm, yopen_pwm_cfg_s *cfg)
{
    if (pwm->config.freq > 0) {
        cfg->period = 1000000 / pwm->config.freq;  /* freq(Hz) to period(us) */
    } else {
        cfg->period = 1000;  /* Default 1ms period */
    }
    
    if (cfg->period > 0) {
        cfg->duty = (pwm->config.pulse * 100) / cfg->period;  /* pulse(us) to duty(%) */
    } else {
        cfg->duty = 50;  /* Default 50% duty */
    }
}

int32_t qm_pwm_init(qm_pwm_dev_t *pwm)
{
    yopen_errcode_pwm ret;
    yopen_pwm_sel channel;
    qm_pwm_priv_t *priv = (qm_pwm_priv_t *)pwm->priv;

    if (pwm == NULL) {
        return -1;
    }

    channel = port_to_channel(pwm->port);
    if (channel >= PWM_MAX) {
        return -1;
    }

    /* Set GPIO alternate function */
    yopen_pin_set_func(pwm->pin.pwm_pin, priv->pin_alt_func);

    ret = yopen_pwm_open(channel);
    if (ret != 0) {
        QM_PWM_DBG("yopen_pwm_open failed: %d", ret);
        return -1;
    }

    QM_PWM_DBG("PWM port %d init success", pwm->port);
    return 0;
}

int32_t qm_pwm_start(qm_pwm_dev_t *pwm)
{
    yopen_errcode_pwm ret;
    yopen_pwm_sel channel;
    yopen_pwm_cfg_s cfg;

    if (pwm == NULL) {
        return -1;
    }

    channel = port_to_channel(pwm->port);
    if (channel >= PWM_MAX) {
        return -1;
    }

    /* Convert config and enable PWM */
    config_to_yopen(pwm, &cfg);
    
    ret = yopen_pwm_enable(channel, &cfg);
    if (ret != 0) {
        QM_PWM_DBG("yopen_pwm_enable failed: %d", ret);
        return -1;
    }

    QM_PWM_DBG("PWM port %d start success, period=%d us, duty=%d%%", 
               pwm->port, cfg.period, cfg.duty);
    return 0;
}

int32_t qm_pwm_stop(qm_pwm_dev_t *pwm)
{
    yopen_errcode_pwm ret;
    yopen_pwm_sel channel;

    if (pwm == NULL) {
        return -1;
    }

    channel = port_to_channel(pwm->port);
    if (channel >= PWM_MAX) {
        return -1;
    }

    ret = yopen_pwm_disable(channel);
    if (ret != 0) {
        QM_PWM_DBG("yopen_pwm_disable failed: %d", ret);
        return -1;
    }

    QM_PWM_DBG("PWM port %d stop success", pwm->port);
    return 0;
}

int32_t qm_pwm_para_change(qm_pwm_dev_t *pwm, qm_pwm_config_t para)
{
    yopen_errcode_pwm ret;
    yopen_pwm_sel channel;
    yopen_pwm_cfg_s cfg;
    uint16_t new_duty;

    if (pwm == NULL) {
        return -1;
    }

    channel = port_to_channel(pwm->port);
    if (channel >= PWM_MAX) {
        return -1;
    }

    /* Update pwm config */
    pwm->config = para;
    
    /* Calculate new duty cycle */
    config_to_yopen(pwm, &cfg);
    new_duty = cfg.duty;

    /* Apply new duty cycle immediately */
    ret = yopen_pwm_duty_set(channel, new_duty);
    if (ret != 0) {
        QM_PWM_DBG("yopen_pwm_duty_set failed: %d", ret);
        return -1;
    }

    QM_PWM_DBG("PWM port %d para changed, pulse=%d, freq=%d, duty=%d%%", 
               pwm->port, para.pulse, para.freq, new_duty);
    return 0;
}

int32_t qm_pwm_fade_start(qm_pwm_dev_t *pwm, uint32_t start_pulse, uint32_t end_pulse, uint32_t timeout)
{
    yopen_errcode_pwm ret;
    yopen_pwm_sel channel;
    yopen_pwm_cfg_s cfg;
    uint16_t start_duty;
    uint16_t end_duty;
    int32_t duty_diff;
    int steps;
    int i;

    if (pwm == NULL) {
        return -1;
    }

    channel = port_to_channel(pwm->port);
    if (channel >= PWM_MAX) {
        return -1;
    }

    /* Get current period config */
    config_to_yopen(pwm, &cfg);
    
    if (cfg.period == 0) {
        return -1;
    }

    /* Convert pulse to duty cycle */
    start_duty = (start_pulse * 100) / cfg.period;
    end_duty = (end_pulse * 100) / cfg.period;

    QM_PWM_DBG("PWM port %d fade start: %d%% -> %d%%, timeout=%dms", 
               pwm->port, start_duty, end_duty, timeout);

    /* Set initial duty cycle */
    ret = yopen_pwm_duty_set(channel, start_duty);
    if (ret != 0) {
        return -1;
    }

    /* Simple linear fade implementation */
    if (start_duty != end_duty && timeout > 0) {
        duty_diff = end_duty - start_duty;
        steps = 10;  /* Use 10 steps for fade */
        uint32_t step_delay = timeout / steps;
        uint32_t fade_step = duty_diff / steps;

        for (i = 1; i <= steps; i++) {
            uint16_t current_duty = start_duty + fade_step * i;
            
            ret = yopen_pwm_duty_set(channel, current_duty);
            if (ret != 0) {
                return -1;
            }
            
            yopen_rtos_task_sleep_ms(step_delay);
        }
    }

    return 0;
}

int32_t qm_pwm_deinit(qm_pwm_dev_t *pwm)
{
    yopen_errcode_pwm ret;
    yopen_pwm_sel channel;

    if (pwm == NULL) {
        return -1;
    }

    channel = port_to_channel(pwm->port);
    if (channel >= PWM_MAX) {
        return -1;
    }

    /* Close PWM */
    ret = yopen_pwm_close(channel);
    if (ret != 0) {
        QM_PWM_DBG("yopen_pwm_close failed: %d", ret);
        return -1;
    }

    QM_PWM_DBG("PWM port %d deinit success", pwm->port);
    return 0;
}
