
#include "luat_base.h"
#include "luat_rtc.h"
#include "sys/time.h"
#include "esp_sleep.h"

#include "luat_log.h"
#define LUAT_LOG_TAG "rtc"

void sntp_set_system_time(uint32_t sec, uint32_t us);

extern int Base_year;

int luat_rtc_set(struct tm *tblock){
    time_t timeSinceEpoch = mktime(tblock);
    struct timeval now = {0};
    now.tv_sec = timeSinceEpoch;
    settimeofday(&now, NULL);
    return 0;
}

int luat_rtc_get(struct tm *tblock){
    time_t now = {0};
    time(&now);
    gmtime_r(&now, tblock);
    return 0;
}

int luat_rtc_timer_start(int id, struct tm *tblock){
    time_t now = {0};
    time(&now);
    time_t time_rtc = mktime(tblock);
    uint64_t time_us = (time_rtc-now)*1000*1000;
    esp_sleep_enable_timer_wakeup(time_us);
    return 0;
}

int luat_rtc_timer_stop(int id){
    return -1; // 暂不支持
}

int luat_rtc_timezone(int* timezone) {
    return 32; // 暂不支持
}

void luat_rtc_set_tamp32(uint32_t tamp) {
    sntp_set_system_time(tamp, 0);
    return;
}
