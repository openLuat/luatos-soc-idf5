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

#define LUAT_LOG_TAG "wlan"
#include "luat_log.h"

static uint8_t wlan_inited = 0;
static uint8_t wlan_is_ready = 0;

static char sta_ip[32];

static int l_wlan_handler(lua_State *L, void* ptr) {
    rtos_msg_t* msg = (rtos_msg_t*)lua_topointer(L, -1);
    int32_t event_id = msg->arg1;
    //esp_netif_ip_info_t ip_info;
    lua_getglobal(L, "sys_pub");
    if (msg->arg2 == 0) {

        switch (event_id)
        {
        case WIFI_EVENT_WIFI_READY:
            LLOGD("wifi protocol is ready");
            lua_getglobal(L, "sys_pub");
            lua_pushstring(L, "WLAN_READY");
            lua_pushinteger(L, 1);
            //esp_netif_get_ip_info(ESP_IF_WIFI_STA,&ip_info);
            lua_call(L, 2, 0);
            break;
        case WIFI_EVENT_STA_CONNECTED:
            LLOGD("wifi connected!!!");
            lua_pushstring(L, "WLAN_STA_CONNECTED");
            lua_call(L, 1, 0);
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            LLOGD("wifi disconnected!!!");
            lua_pushstring(L, "WLAN_STA_DISCONNECTED");
            lua_call(L, 1, 0);
            lua_getglobal(L, "sys_pub");
            lua_pushstring(L, "WLAN_READY");
            lua_pushinteger(L, 0);
            lua_call(L, 2, 0);
            break;
        case WIFI_EVENT_STA_START:
            //LLOGD("wifi station start");
            break;
        case WIFI_EVENT_STA_STOP:
            //LLOGD("wifi station stop");
            break;
        case WIFI_EVENT_SCAN_DONE:
            LLOGD("wifi scan done");
            lua_pushstring(L, "WLAN_SCAN_DONE");
            lua_call(L, 1, 0);
            break;
        default:
            LLOGI("unkown event %d", event_id);
            break;
        }
    }
    else if (msg->arg2 == 1) {
        if (event_id == IP_EVENT_STA_GOT_IP) {
            LLOGD("IP_EVENT_STA_GOT_IP %s", sta_ip);
            lua_pushstring(L, "IP_READY");
            lua_pushstring(L, sta_ip);
            lua_call(L, 2, 0);
        }
    }
    return 0;
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data) {
    rtos_msg_t msg = {0};
    msg.handler = l_wlan_handler;
    msg.arg1 = event_id;

    LLOGD("wifi event %d", event_id);
    if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wlan_is_ready = 0;
    }
    luat_msgbus_put(&msg, 0);
}

static void ip_event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data) {
    rtos_msg_t msg = {0};
    msg.handler = l_wlan_handler;
    msg.arg1 = event_id;
    msg.arg2 = 1;
    ip_event_got_ip_t *event;

    LLOGD("ip event %d", event_id);
    if (event_id == IP_EVENT_STA_GOT_IP) {
        wlan_is_ready = 1;
        event = (ip_event_got_ip_t*)event_data;
        sprintf(sta_ip, IPSTR, IP2STR(&event->ip_info.ip));
    }
    luat_msgbus_put(&msg, 0);
}

int luat_wlan_init(luat_wlan_config_t *conf) {
    if (wlan_inited != 0) {
        LLOGI("wlan is ready!!");
        return 0;
    }

    esp_netif_init();
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT,   ESP_EVENT_ANY_ID,   &ip_event_handler, NULL);
    esp_event_handler_register(SC_EVENT,   ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    cfg.static_rx_buf_num = 2;
    cfg.static_tx_buf_num = 2;

    int ret = esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_STA);
    LLOGD("esp_wifi_init ret %d", ret);
    ret = esp_wifi_start();
    LLOGD("esp_wifi_start ret %d", ret);
    wlan_inited = 1;
    return 0;
}

// 设置wifi模式
int luat_wlan_mode(luat_wlan_config_t *conf) {
    switch (conf->mode)
    {
    case LUAT_WLAN_MODE_NULL:
        esp_wifi_set_mode(WIFI_MODE_NULL);
        break;
    case LUAT_WLAN_MODE_STA:
        esp_wifi_set_mode(WIFI_MODE_STA);
        break;
    case LUAT_WLAN_MODE_AP:
        esp_wifi_set_mode(WIFI_MODE_AP);
        break;
    case LUAT_WLAN_MODE_APSTA:
        esp_wifi_set_mode(WIFI_MODE_APSTA);
        break;
    }
    return 0;
}

// 是否已经连上wifi
int luat_wlan_ready(void) {
    return wlan_is_ready;
}

int luat_wlan_connect(luat_wlan_conninfo_t* info) {
    int ret = 0;

    wifi_config_t cfg = {0};

    // LLOGD("connect %s %s", info->ssid, info->password);

    memcpy(cfg.sta.ssid, info->ssid, strlen(info->ssid));
    memcpy(cfg.sta.password, info->password, strlen(info->password));
    esp_wifi_set_config(WIFI_IF_STA, &cfg);
    ret = esp_wifi_connect();
    LLOGD("esp_wifi_connect ret %d", ret);
    return 0;
}

int luat_wlan_disconnect(void) {
    int ret = 0;
    ret = esp_wifi_disconnect();
    LLOGD("esp_wifi_disconnect ret %d", ret);
    return 0;
}

int luat_wlan_scan(void) {
    static const wifi_scan_config_t conf = {0};
    return esp_wifi_scan_start(&conf, false);
}

int luat_wlan_scan_get_result(luat_wlan_scan_result_t *results, int ap_limit) {
    uint16_t num = ap_limit;
    luat_wlan_scan_result_t *tmp = results;
    wifi_ap_record_t *ap_info = luat_heap_malloc(sizeof(wifi_ap_record_t) * ap_limit);
    if (ap_info == NULL)
        return 0;
    esp_wifi_scan_get_ap_records(&num, ap_info);
    for (size_t i = 0; i < num; i++)
    {
        memcpy(tmp[i].ssid, ap_info[i].ssid, strlen((const char*)ap_info[i].ssid));
        memcpy(tmp[i].bssid, ap_info[i].bssid, 6);
        tmp[i].rssi = ap_info[i].rssi;
    }
    luat_heap_free(ap_info);
    return num;
}

