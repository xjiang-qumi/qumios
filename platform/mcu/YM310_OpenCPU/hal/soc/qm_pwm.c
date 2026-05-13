#include "qm_pwm.h"
#include "hal/pwm.h"
#include "qm_errno.h"

#define QM_PWM_DBG(...) //QM_LOGD("PWM", __VA_ARGS__)

int32_t qm_pwm_init(qm_pwm_dev_t *pwm)
{
    PWM_CYCLE_RANGE_E cycle_range;
    UINT32 high_time, cycle_time;

    if (pwm == NULL) {
        return -QM_EINVAL;
    }

    if (pwm->config.freq > 1000) {
        cycle_range = PWM_CYCLE_ABOVE_1US;
    } else if (pwm->config.freq > 100) {
        cycle_range = PWM_CYCLE_ABOVE_10US;
    } else {
        cycle_range = PWM_CYCLE_ABOVE_1MS;
    }

    cycle_time = 1000000 / pwm->config.freq;
    high_time = pwm->config.pulse;

    if (hal_PwmConfig(pwm->pin.pwm_pin, cycle_range, high_time, cycle_time) != 0) {
        return -QM_EIO;
    }

    QM_PWM_DBG("PWM port %d init success", pwm->port);
    return QM_EOK;
}

int32_t qm_pwm_start(qm_pwm_dev_t *pwm)
{
    if (pwm == NULL) {
        return -QM_EINVAL;
    }

    if (hal_PwmEnable(pwm->pin.pwm_pin) != 0) {
        return -QM_EIO;
    }

    QM_PWM_DBG("PWM port %d start success", pwm->port);
    return QM_EOK;
}

int32_t qm_pwm_stop(qm_pwm_dev_t *pwm)
{
    if (pwm == NULL) {
        return -QM_EINVAL;
    }

    if (hal_PwmDisable(pwm->pin.pwm_pin) != 0) {
        return -QM_EIO;
    }

    QM_PWM_DBG("PWM port %d stop success", pwm->port);
    return QM_EOK;
}

int32_t qm_pwm_para_change(qm_pwm_dev_t *pwm, qm_pwm_config_t para)
{
    if (pwm == NULL) {
        return -QM_EINVAL;
    }

    pwm->config = para;

    return qm_pwm_init(pwm);
}

int32_t qm_pwm_fade_start(qm_pwm_dev_t *pwm, uint32_t start_pulse, uint32_t end_pulse, uint32_t timeout)
{
    (void)pwm;
    (void)start_pulse;
    (void)end_pulse;
    (void)timeout;

    return -QM_ERROR;
}

int32_t qm_pwm_deinit(qm_pwm_dev_t *pwm)
{
    if (pwm == NULL) {
        return -QM_EINVAL;
    }

    hal_PwmDisable(pwm->pin.pwm_pin);
    QM_PWM_DBG("PWM port %d deinit success", pwm->port);
    return QM_EOK;
}