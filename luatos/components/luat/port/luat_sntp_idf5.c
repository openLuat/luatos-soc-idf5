#include "luat_base.h"
#include "luat_msgbus.h"
#include "luat_wlan.h"


#include "esp_attr.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_smartconfig.h"
#include "esp_mac.h"

#include <time.h>
#include <sys/time.h>

#define LUAT_LOG_TAG "sntp"
#include "luat_log.h"

static uint8_t ntp_auto_sync_started = 0; 
static uint8_t ntp_first_ntp = 0; 

static int l_ntp_sync_cb(lua_State *L, void* ptr) {
    lua_getglobal(L, "sys_pub");
    if (!lua_isnil(L, -1)) {
        lua_pushstring(L, "NTP_UPDATE");
        lua_call(L, 1, 0);
    }
    return 0;
}
    
static void my_ntp_cb (struct timeval *tv) {
    if (ntp_first_ntp == 0) {
        ntp_first_ntp = 1;
        LLOGD("time sync done");
    }

    rtos_msg_t msg = {0};
    msg.handler = l_ntp_sync_cb;
    luat_msgbus_put(&msg, 0);
}

// 自动进行NTP同步
#include "esp_sntp.h"
void luat_ntp_autosync(void) {
    if (ntp_auto_sync_started != 0) {
        return;
    }

    ntp_auto_sync_started = 1;

    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "ntp.aliyun.com");
    esp_sntp_setservername(1, "pool.ntp.org");
#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
    esp_sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
#endif
    esp_sntp_set_sync_interval(900*1000); // every 60s
    esp_sntp_set_time_sync_notification_cb(my_ntp_cb);
    esp_sntp_init();
}
