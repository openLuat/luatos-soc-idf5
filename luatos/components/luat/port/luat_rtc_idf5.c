
#include "luat_base.h"
#include "luat_rtc.h"
#include "sys/time.h"

#include "luat_log.h"
#define LUAT_LOG_TAG "rtc"

extern int Base_year;

int luat_rtc_set(struct tm *tblock){
    tblock->tm_year -= Base_year;
    tblock->tm_mon--;
    time_t timeSinceEpoch = mktime(tblock);
    struct timeval now = {0};
    now.tv_sec = timeSinceEpoch;
    settimeofday(&now, NULL);
    return 0;
}

int luat_rtc_get(struct tm *tblock){
    time_t now = {0};
    time(&now);
    localtime_r(&now, tblock);
    tblock->tm_year += Base_year;
    tblock->tm_mon++;
    return 0;
}

int luat_rtc_timer_start(int id, struct tm *tblock){
    return -1; // 暂不支持
}

int luat_rtc_timer_stop(int id){
    return -1; // 暂不支持
}
