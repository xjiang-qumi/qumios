#include "qm.h"
#include "qm_time.h"
#include "qm_rtc.h"
#include "time_api.h"

int32_t qm_rtc_init(qm_rtc_dev_t *rtc)
{
    return QM_EOK;
}


// 根据 t_rtc 计算星期几
// 返回值：0=星期日, 1=星期一, 2=星期二, 3=星期三, 4=星期四, 5=星期五, 6=星期六
static int rtc_get_weekday(t_rtc *t) {
    int year = t->tm_year + 1970;  // t_rtc.tm_year 是自1970年以来的年数
    int month = t->tm_mon;
    int day = t->tm_mday;
    
    if (month == 1 || month == 2) {
        month += 12;
        year--;
    }
    
    int century = year / 100;
    int year_of_century = year % 100;
    
    int weekday = (day + 13 * (month + 1) / 5 + year_of_century + year_of_century / 4 + century / 4 + 5 * century) % 7;
    
    return (weekday + 6) % 7;
}

int32_t qm_rtc_get_time(qm_rtc_dev_t *rtc, qm_rtc_time_t *t)
{
    int wday;
    t_rtc tm;
    if(rtc == NULL || t == NULL){
        return -QM_EINVAL;
    }

    time_t currSeconds = time(NULL);
    pmic_rtc_to_tm((int)currSeconds, &tm);

    t->year = tm.tm_year + 1970;
    t->month = tm.tm_mon;
    t->date = tm.tm_mday;
    t->hr = tm.tm_hour;
    t->min = tm.tm_min;
    t->sec = tm.tm_sec;

    wday = rtc_get_weekday(&tm);
    if(wday == 0){
        t->weekday = 7;
    }else{
        t->weekday = wday;
    } 
    return QM_EOK;
}

int32_t qm_rtc_set_time(qm_rtc_dev_t *rtc, const qm_rtc_time_t *time)
{
    t_rtc tm;
    if(rtc == NULL || time == NULL){
        return -QM_EINVAL;
    }

    tm.tm_year = time->year - 1970;  // t_rtc.tm_year 是自1970年以来的年数
    tm.tm_mon = time->month;
    tm.tm_mday = time->date;
    tm.tm_hour = time->hr;
    tm.tm_min = time->min;
    tm.tm_sec = time->sec;

    lte_module_time_set(&tm, NULL);
    return QM_EOK;
}

int32_t qm_rtc_get_timestamp(qm_rtc_dev_t *rtc, uint32_t *timestamp)
{
    if(rtc == NULL || timestamp == NULL){
        return -QM_EINVAL;
    }
    *timestamp = (uint32_t)time(NULL);
    return QM_EOK;
}

int32_t qm_rtc_set_timestamp(qm_rtc_dev_t *rtc, uint32_t timestamp)
{    
    t_rtc tm;
    if(rtc == NULL){
        return -QM_EINVAL;
    }

    pmic_rtc_to_tm((int)timestamp, &tm);
    lte_module_time_set(&tm, NULL);

    return QM_EOK;
}

int32_t qm_rtc_deinit(qm_rtc_dev_t *rtc)
{  
    return QM_EOK; 
}