#include "qm.h"
#include "qm_work.h"
#include "qm_log.h"

#define QM_TEST_EVENT 0x2000 
#define QM_TEST_SUB_EVENT 0x0001

static char *test_arg = "hello";
static qm_work_t work ={0};

#define LOG_TAG "qm_work"

void test_event_callback(qm_input_event_t *input_event, void *arg)
{
    QM_LOGD("LOG_TAG", "event: 0x%04x, sub_event: 0x%04x, arg: %s", input_event->event, input_event->sub_event, (char*)arg);  
    QM_LOGD("LOG_TAG", "value: %s", input_event->value);  
}

void test_action_callback(void *arg)
{
    QM_LOGD("LOG_TAG", "arg: %s", arg);
    qm_post_delayed_action(&work, test_action_callback, test_arg, 1000);
}

void qm_application_start(void)
{
    int ret = 0;
    char *value = "world";
    // ret = qm_event_register(QM_TEST_EVENT, test_event_callback, test_arg);
    // QM_LOGD("LOG_TAG", "ret: %d", ret);
    // ret = qm_event_post(QM_TEST_EVENT, QM_TEST_SUB_EVENT, value, strlen(value) + 1);
    // QM_LOGD("LOG_TAG", "ret: %d", ret);

    qm_post_delayed_action(&work, test_action_callback, test_arg, 1000);
}





