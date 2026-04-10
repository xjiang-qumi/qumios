#include "qm.h"
#include "qm_time.h"
#include "qm_rtc.h"
#include "yopen_rtc.h"

int32_t qm_rtc_init(qm_rtc_dev_t *rtc)
{
    return QM_EOK;
}


// 根据 yopen_rtc_time_t 计算星期几
// 返回值：0=星期日, 1=星期一, 2=星期二, 3=星期三, 4=星期四, 5=星期五, 6=星期六
static int yopen_get_weekday(yopen_rtc_time_t *t) {
    int year = t->tm_year;
    int month = t->tm_mon;
    int day = t->tm_mday;
    
    // 基姆拉尔森计算公式 (处理1、2月的情况)
    if (month == 1 || month == 2) {
        month += 12;
        year--;
    }
    
    int century = year / 100;      // 世纪数
    int year_of_century = year % 100; // 年份的后两位
    
    // 公式：weekday = (day + 13*(month+1)/5 + year_of_century + year_of_century/4 + century/4 + 5*century) % 7
    int weekday = (day + 13 * (month + 1) / 5 + year_of_century + year_of_century / 4 + century / 4 + 5 * century) % 7;
    
    // 公式结果转换为标准表示（0=星期日，1=星期一，...）
    return (weekday + 6) % 7; // 调整结果使 0=星期日
}

int32_t qm_rtc_get_time(qm_rtc_dev_t *rtc, qm_rtc_time_t *time)
{
    int wday;
    yopen_rtc_time_t tm;
    yopen_errcode_rtc_e ret;
    if(rtc == NULL || time == NULL){
        return -QM_EINVAL;
    }
    ret = yopen_rtc_get_time(&tm);
    if(ret != 0){
        return -QM_ERROR;
    }

    time->year = tm.tm_year + 2000;
    time->month = tm.tm_mon;
    time->date = tm.tm_mday;
    time->hr = tm.tm_hour;
    time->min = tm.tm_min;
    time->sec = tm.tm_sec;

    wday = yopen_get_weekday(&tm);
    if(wday == 0){
        time->weekday = 7;
    }else{
        time->weekday = wday;
    } 
    return QM_EOK;
}

int32_t qm_rtc_set_time(qm_rtc_dev_t *rtc, const qm_rtc_time_t *time)
{
    yopen_rtc_time_t tm;
    yopen_errcode_rtc_e ret;
    if(rtc == NULL || time == NULL){
        return -QM_EINVAL;
    }

    tm.tm_year = time->year - 2000; 
    tm.tm_mon = time->month; 
    tm.tm_mday = time->date;  
    tm.tm_hour = time->hr;
    tm.tm_min = time->min;  
    tm.tm_sec = time->sec;
    ret = yopen_rtc_set_time(&tm);
    if(ret != 0){
        return -QM_ERROR;
    }
    return QM_EOK;
}

int32_t qm_rtc_get_timestamp(qm_rtc_dev_t *rtc, uint32_t *timestamp)
{
    if(rtc == NULL || timestamp == NULL){
        return -QM_EINVAL;
    }
    *timestamp = yopen_rtc_get_time_s();
    return QM_EOK;
}

int32_t qm_rtc_set_timestamp(qm_rtc_dev_t *rtc, uint32_t timestamp)
{    
    struct qm_tm tm;
    qm_time_t time;
    yopen_rtc_time_t yopen_tm;
    yopen_errcode_rtc_e ret;

    if(rtc == NULL){
        return -QM_EINVAL;
    }

    time = (qm_time_t)timestamp;
    qm_gmtime_r(&time, &tm);
    yopen_tm.tm_year =  tm.tm_year + 1900 - 2000;
    yopen_tm.tm_mon = tm.tm_mon + 1;
    yopen_tm.tm_mday = tm.tm_mday;
    yopen_tm.tm_hour = tm.tm_hour;
    yopen_tm.tm_min = tm.tm_min;
    yopen_tm.tm_sec = tm.tm_sec;

    ret = yopen_rtc_set_time(&yopen_tm);
    if(ret != 0){
        return -QM_ERROR;
    }

    return QM_EOK;
}

int32_t qm_rtc_deinit(qm_rtc_dev_t *rtc)
{  
    return QM_EOK; 
}