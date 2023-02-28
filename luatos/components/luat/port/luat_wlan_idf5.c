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

#define LUAT_LOG_TAG "wlan"
#include "luat_log.h"

#include "lwip/netif.h"
#include "luat_network_adapter.h"
#include "luat_timer.h"
#include "net_lwip.h"
#include "lwip/tcp.h"

void net_lwip_set_link_state(uint8_t adapter_index, uint8_t updown);

static uint8_t wlan_inited = 0;
static uint8_t wlan_is_ready = 0;

static smartconfig_event_got_ssid_pswd_t *sc_evt;

static char sta_ip[32];
static char sta_gw[32];
static char sta_connected_bssid[6];

static uint8_t smartconfig_state = 0; // 0 - idle, 1 - running
static uint8_t auto_reconnection = 0;

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
            if (auto_reconnection && smartconfig_state == 0) {
                LLOGD("auto reconnecting ...");
                esp_wifi_connect();
            }
            break;
        case WIFI_EVENT_STA_START:
            LLOGD("wifi station start");
            break;
        case WIFI_EVENT_STA_STOP:
            LLOGD("wifi station stop");
            break;
        case WIFI_EVENT_SCAN_DONE:
            LLOGD("wifi scan done");
            lua_pushstring(L, "WLAN_SCAN_DONE");
            lua_call(L, 1, 0);
            break;
        case WIFI_EVENT_AP_START:
            LLOGD("wifi ap start");
            break;
        case WIFI_EVENT_AP_STOP:
            LLOGD("wifi ap stop");
            break;
        case WIFI_EVENT_AP_STACONNECTED :
            LLOGD("wifi ap sta connected");
            break;
        case WIFI_EVENT_AP_STADISCONNECTED :
            LLOGD("wifi ap sta disconnected");
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
            lua_pushinteger(L, NW_ADAPTER_INDEX_LWIP_WIFI_STA);
            lua_call(L, 3, 0);
        }
    }
    else if (msg->arg2 == 2) {
        if (event_id == SC_EVENT_SCAN_DONE) {
            LLOGD("smartconfig Scan done");
        }
        else if (event_id == SC_EVENT_SCAN_DONE) {
            LLOGD("smartconfig Found channel");
        }
        else if (event_id == SC_EVENT_GOT_SSID_PSWD) {
            LLOGD("smartconfig Got ssid and password");
            smartconfig_event_got_ssid_pswd_t *evt = sc_evt;
            wifi_config_t wifi_config;
            uint8_t ssid[33] = { 0 };
            uint8_t password[65] = { 0 };
            // uint8_t rvd_data[33] = { 0 };

            bzero(&wifi_config, sizeof(wifi_config_t));
            memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
            memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));
            wifi_config.sta.bssid_set = evt->bssid_set;
            if (wifi_config.sta.bssid_set == true) {
                memcpy(wifi_config.sta.bssid, evt->bssid, sizeof(wifi_config.sta.bssid));
            }

            memcpy(ssid, evt->ssid, sizeof(evt->ssid));
            memcpy(password, evt->password, sizeof(evt->password));
            LLOGD("SSID [%s] PASSWORD [%s]", ssid, password);
            // ESP_LOGI(TAG, "SSID:%s", ssid);
            // ESP_LOGI(TAG, "PASSWORD:%s", password);
            // if (evt->type == SC_TYPE_ESPTOUCH_V2) {
            //     ESP_ERROR_CHECK( esp_smartconfig_get_rvd_data(rvd_data, sizeof(rvd_data)) );
            //     ESP_LOGI(TAG, "RVD_DATA:");
            //     for (int i=0; i<33; i++) {
            //         printf("%02x ", rvd_data[i]);
            //     }
            //     printf("\n");
            // }

            esp_wifi_disconnect();
            esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
            esp_wifi_connect();

            lua_pushstring(L, "SC_RESULT");
            lua_pushstring(L, (const char*)ssid);
            lua_pushstring(L, (const char*)password);
            lua_call(L, 3, 0);
        }
        else if (event_id == SC_EVENT_SEND_ACK_DONE) {
            esp_smartconfig_stop();
            smartconfig_state = 0;
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
#ifdef LUAT_USE_NETWORK
        net_lwip_set_link_state(NW_ADAPTER_INDEX_LWIP_WIFI_STA, 0);
#endif
        // wifi_event_sta_disconnected_t* sta = (wifi_event_sta_disconnected_t*)event_data;
        memset(sta_connected_bssid, 0, sizeof(sta_connected_bssid));
    }
    if (event_id == WIFI_EVENT_STA_CONNECTED) {
        wifi_event_sta_connected_t *sta = (wifi_event_sta_connected_t*)event_data;
        memcpy(sta_connected_bssid, sta->bssid, 6);
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
#ifdef LUAT_USE_NETWORK
        // posix_network_set_ready(1);
        net_lwip_set_link_state(NW_ADAPTER_INDEX_LWIP_WIFI_STA, 1);
#endif
        event = (ip_event_got_ip_t*)event_data;
        sprintf(sta_ip, IPSTR, IP2STR(&event->ip_info.ip));
        sprintf(sta_gw, IPSTR, IP2STR(&event->ip_info.gw));
    }
    luat_msgbus_put(&msg, 0);
}

static void sc_event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data) {
    rtos_msg_t msg = {0};
    msg.handler = l_wlan_handler;
    msg.arg1 = event_id;
    msg.arg2 = 2;
    
    if (event_id == SC_EVENT_GOT_SSID_PSWD && sc_evt != NULL) {
        memcpy(sc_evt, event_data, sizeof(smartconfig_event_got_ssid_pswd_t));
    }

    LLOGD("sc event %d", event_id);
    luat_msgbus_put(&msg, 0);
}

extern void luat_ntp_autosync(void);
int luat_wlan_init(luat_wlan_config_t *conf) {
    int ret = 0;
    if (wlan_inited == 0) {
        esp_netif_init();
        esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
        esp_event_handler_register(IP_EVENT,   ESP_EVENT_ANY_ID, &ip_event_handler,   NULL);
        esp_event_handler_register(SC_EVENT,   ESP_EVENT_ANY_ID, &sc_event_handler,   NULL);

        esp_netif_create_default_wifi_sta();

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        cfg.static_rx_buf_num = 2;
        cfg.static_tx_buf_num = 2;

        ret = esp_wifi_init(&cfg);
        esp_wifi_set_mode(WIFI_MODE_STA);

        #ifdef LUAT_USE_NETWORK
        // luat_timer_mdelay(3000);
        LLOGD("CALL net_lwip_init");
        net_lwip_init();
        // luat_timer_mdelay(3000);
        LLOGD("CALL net_lwip_register_adapter");
        net_lwip_register_adapter(NW_ADAPTER_INDEX_LWIP_WIFI_STA);
        // luat_timer_mdelay(3000);
        #endif
        
        struct netif *et0 = netif_get_by_index(1);
        // LLOGD("netif_get_by_index %p", et0);
        // luat_timer_mdelay(3000);
        extern void net_lwip_set_netif(uint8_t adapter_index, struct netif *netif, void *init, uint8_t is_default);
        net_lwip_set_netif(NW_ADAPTER_INDEX_LWIP_WIFI_STA, et0, NULL, 1);
        LLOGD("esp_wifi_init ret %d", ret);
    }
#ifdef LUAT_USE_NIMBLE
#if CONFIG_BT_ENABLED
    //esp_wifi_set_ps(WIFI_PS_NONE);
#endif
#endif
    ret = esp_wifi_start();
    LLOGD("esp_wifi_start ret %d", ret);
    
    wlan_inited = 1;

    // 自动开启ntp
    luat_ntp_autosync();

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
    // 允许重用一起的ssid和passwd数据直接重连
    if (strlen(info->ssid)) {
        // LLOGD("connect %s %s", info->ssid, info->password);
        memcpy(cfg.sta.ssid, info->ssid, strlen(info->ssid));
        memcpy(cfg.sta.password, info->password, strlen(info->password));
        esp_wifi_set_config(WIFI_IF_STA, &cfg);
    }
    auto_reconnection = 1;
    
    ret = esp_wifi_connect();
    LLOGD("esp_wifi_connect ret %d", ret);
    return 0;
}

int luat_wlan_disconnect(void) {
    int ret = 0;
    auto_reconnection = 0;
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

int luat_wlan_smartconfig_start(int tp) {
    if (smartconfig_state != 0) {
        LLOGI("smartconfig is running");
        return 0;
    }
    switch (tp)
    {
    case LUAT_SC_TYPE_ESPTOUCH:
        esp_smartconfig_set_type(SC_TYPE_ESPTOUCH);
        break;
    case LUAT_SC_TYPE_ESPTOUCH_AIRKISS:
        esp_smartconfig_set_type(SC_TYPE_ESPTOUCH_AIRKISS);
        break;
    case LUAT_SC_TYPE_AIRKISS:
        esp_smartconfig_set_type(SC_TYPE_AIRKISS);
        break;
    case LUAT_SC_TYPE_ESPTOUCH_V2:
        esp_smartconfig_set_type(SC_TYPE_ESPTOUCH_V2);
        break;
    default:
        esp_smartconfig_set_type(SC_TYPE_ESPTOUCH);
        break;
    }
    if (sc_evt == NULL) {
        sc_evt = luat_heap_malloc(sizeof(smartconfig_event_got_ssid_pswd_t));
    }
    smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
    esp_smartconfig_start(&cfg);
    smartconfig_state = 1;
    return 0;
}

int luat_wlan_smartconfig_stop(void) {
    esp_smartconfig_stop();
    smartconfig_state = 0;
    return 0;
}

int luat_wlan_get_mac(int id, char* mac) {
    if (id >= 0 && id <= ESP_MAC_IEEE802154) {
        esp_read_mac((uint8_t*)mac, id);
        return 0;
    }
    else {
        LLOGW("no such mac id %d", id);
        return -1;
    }
}

int luat_wlan_set_mac(int id, char* mac) {
    if (id >= 0 && id <= ESP_MAC_IEEE802154) {
        esp_base_mac_addr_set((uint8_t*)mac);
        return 0;
    }
    else {
        LLOGW("no such mac id %d", id);
        return -1;
    }
}

static uint8_t ap_stack_inited = 0;

int luat_wlan_ap_start(luat_wlan_apinfo_t *apinfo) {
    wifi_mode_t mode = 0;
    int ret = 0;
    wifi_config_t cfg = {0};

    cfg.ap.channel = 1;
    cfg.ap.max_connection = 5;

    memcpy(cfg.ap.ssid, apinfo->ssid, strlen(apinfo->ssid));
    cfg.ap.ssid_len = strlen(apinfo->ssid);
    if (strlen(apinfo->password) >= 6) {
        memcpy(cfg.ap.password, apinfo->password, strlen(apinfo->password));
        cfg.ap.authmode = WIFI_AUTH_WPA2_PSK;
    }
    else {
        cfg.ap.authmode = WIFI_AUTH_OPEN;
    }

    LLOGD("softap %s %s", apinfo->ssid, apinfo->password);

    ret = esp_wifi_stop();
    LLOGD("esp_wifi_stop ret %d", ret);

    if (ap_stack_inited == 0) {
        ap_stack_inited = 1;
        esp_netif_create_default_wifi_ap();
    }

    ret = esp_wifi_get_mode(&mode);
    if (mode == WIFI_MODE_NULL || mode == WIFI_MODE_STA) {
        LLOGI("auto set to APSTA mode");
        ret = esp_wifi_set_mode(WIFI_MODE_APSTA);
        LLOGD("esp_wifi_set_mode ret %d", ret);
    }
    ret = esp_wifi_set_config(WIFI_IF_AP, &cfg);
    LLOGD("esp_wifi_set_config ret %d", ret);
    ret = esp_wifi_start();
    LLOGD("esp_wifi_start ret %d", ret);
    return 0;
}

int luat_wlan_get_ip(int type, char* data) {
    memcpy(data, sta_ip, strlen(sta_ip) + 1);
    return 0;
}

int luat_wlan_set_ps(int mode) {
    if (mode == 0) {
        esp_wifi_set_ps(WIFI_PS_NONE);
    }
    else if (mode == 1) {
        esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
    }
    else if (mode == 2) {
        esp_wifi_set_ps(WIFI_PS_MAX_MODEM);
    }
    else {
        LLOGE("unkown wifi ps mode %d", mode);
        return -1;
    }
    return 0;
}

int luat_wlan_get_ps(void) {
    wifi_ps_type_t mode = WIFI_PS_NONE;
    esp_wifi_get_ps(&mode);
    return mode;
}

int luat_wlan_get_ap_bssid(char* buff) {
    memcpy(buff, sta_connected_bssid, 6);
    return 0;
}

int luat_wlan_get_ap_rssi(void) {
    wifi_ap_record_t ap;
    esp_wifi_sta_get_ap_info(&ap);
    return ap.rssi;
}

int luat_wlan_get_ap_gateway(char* buff) {
    memcpy(buff, sta_gw, strlen(sta_gw) + 1);
    return 0;
}
