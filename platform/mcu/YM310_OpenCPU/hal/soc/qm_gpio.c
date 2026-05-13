#include "qm.h"
#include "qm_gpio.h"
#include "qm_errno.h"
#include "hal/gpio.h"

int32_t qm_gpio_init(qm_gpio_dev_t *gpio)
{
    GPIOReturnCode ret;
    GPIOConfiguration config = {0};

    if (gpio == NULL) {
        return -QM_EINVAL;
    }

    switch (gpio->config.mode) {
        case QM_GPIO_MODE_INPUT:
            config.pinDir = GPIO_IN_PIN;
            break;
        case QM_GPIO_MODE_OUTPUT:
            config.pinDir = GPIO_OUT_PIN;
            break;
        case QM_GPIO_MODE_OUTPUT_OD:
            config.pinDir = GPIO_OUT_PIN;
            break;
        case QM_GPIO_MODE_INPUT_OUTPUT_OD:
        case QM_GPIO_MODE_INPUT_OUTPUT:
            config.pinDir = GPIO_OUT_PIN;
            break;
        case QM_GPIO_MODE_DISABLE:
        default:
            return -QM_EIO;
    }

    switch (gpio->config.pull_en) {
        case QM_GPIO_PULLUP_ONLY:
            config.pinPull = GPIO_PULLUP_ENABLE;
            break;
        case QM_GPIO_PULLDOWN_ONLY:
            config.pinPull = GPIO_PULLDN_ENABLE;
            break;
        case QM_GPIO_PULLUP_PULLDOWN:
            config.pinPull = GPIO_PULLUP_ENABLE;
            break;
        case QM_GPIO_FLOATING:
        default:
            config.pinPull = GPIO_PULL_DISABLE;
            break;
    }

    ret = hal_ConfigGpio(gpio->port, config);
    if (ret != GPIORC_OK) {
        return -QM_EIO;
    }

    return QM_EOK;
}

int32_t qm_gpio_set_level(qm_gpio_dev_t *gpio, uint8_t level)
{
    if (gpio == NULL) {
        return -QM_EINVAL;
    }

    hal_GpioOut(gpio->port, level);
    return QM_EOK;
}

uint8_t qm_gpio_get_level(qm_gpio_dev_t *gpio)
{
    GPIOReturnCode ret;

    if (gpio == NULL) {
        return -QM_EINVAL;
    }

    ret = hal_GpioIn(gpio->port);
    return (ret == GPIORC_HIGH) ? 1 : 0;
}

int32_t qm_gpio_enable_irq(qm_gpio_dev_t *gpio, qm_gpio_intr_type_t intr_type, qm_gpio_irq_handler_t handler, void *arg)
{
    GPIOConfiguration config = {0};
    GPIOReturnCode ret;

    if (gpio == NULL || handler == NULL) {
        return -QM_EINVAL;
    }

    config.pinDir = GPIO_IN_PIN;
    config.isr = (GPIOCallback)handler;

    switch (intr_type) {
        case QM_GPIO_INTR_POSEDGE:
            config.pinEd = GPIO_RISE_EDGE;
            break;
        case QM_GPIO_INTR_NEGEDGE:
            config.pinEd = GPIO_FALL_EDGE;
            break;
        case QM_GPIO_INTR_ANYEDGE:
            config.pinEd = GPIO_TWO_EDGE;
            break;
        case QM_GPIO_INTR_LOW_LEVEL:
        case QM_GPIO_INTR_HIGH_LEVEL:
            config.pinEd = GPIO_TWO_EDGE;
            break;
        case QM_GPIO_INTR_DISABLE:
        default:
            return -QM_EINVAL;
    }

    switch (gpio->config.pull_en) {
        case QM_GPIO_PULLUP_ONLY:
            config.pinPull = GPIO_PULLUP_ENABLE;
            break;
        case QM_GPIO_PULLDOWN_ONLY:
            config.pinPull = GPIO_PULLDN_ENABLE;
            break;
        case QM_GPIO_PULLUP_PULLDOWN:
            config.pinPull = GPIO_PULLUP_ENABLE;
            break;
        case QM_GPIO_FLOATING:
        default:
            config.pinPull = GPIO_PULL_DISABLE;
            break;
    }

    ret = hal_ConfigGpio(gpio->port, config);
    if (ret != GPIORC_OK) {
        return -QM_EIO;
    }

    return QM_EOK;
}

int32_t qm_gpio_disable_irq(qm_gpio_dev_t *gpio)
{
    if (gpio == NULL) {
        return -QM_EINVAL;
    }

    hal_GpioDisableEdgeDet(gpio->port, GPIO_TWO_EDGE);
    return QM_EOK;
}

int32_t qm_gpio_clear_irq(qm_gpio_dev_t *gpio)
{
    (void)gpio;
    return QM_EOK;
}

int32_t qm_gpio_deinit(qm_gpio_dev_t *gpio)
{
    (void)gpio;
    return QM_EOK;
}