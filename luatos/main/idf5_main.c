
#include "luat_base.h"
#include "luat_timer.h"

#include <string.h>
#include <stdlib.h>
#include "esp_event.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "luat_rtos.h"

#include <time.h>
#include <sys/time.h>

#define LUAT_LOG_TAG "main"
#include "luat_log.h"

#ifdef LUAT_USE_LVGL
#include "lvgl.h"
#include "luat_lvgl.h"
#endif

extern int luat_main (void);
extern void bootloader_random_enable(void);
extern void luat_heap_init(void);

#ifdef LUAT_USE_LVGL
static void luat_lvgl_callback(TimerHandle_t xTimer){
    lv_tick_inc(10);
    rtos_msg_t msg = {0};
    msg.handler = lv_task_handler;
    luat_msgbus_put(&msg, 0);
}
#endif

#ifdef LUAT_USE_NETWORK
#include "luat_network_adapter.h"
#endif

void app_main(void){
    void luat_mcu_us_timer_init();
    luat_mcu_us_timer_init();
#ifdef LUAT_USE_SHELL
    extern void luat_shell_poweron(int _drv);
	luat_shell_poweron(0);
#endif
    bootloader_random_enable();
    esp_err_t r = nvs_flash_init();
    if (r == ESP_ERR_NVS_NO_FREE_PAGES || r == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        //ESP_LOGI("no free pages or nvs version mismatch, erase...");
        nvs_flash_erase();
        r = nvs_flash_init();
    }

    setenv("TZ", "CST-8", 1);
    tzset();

    luat_heap_init();
    esp_event_loop_create_default();
#ifdef LUAT_USE_LVGL
    lv_init();
    TimerHandle_t os_timer = xTimerCreate("lvgl_timer", 10 / portTICK_PERIOD_MS, true, NULL, luat_lvgl_callback);
    xTimerStart(os_timer, 0);
#endif

    luat_main();
}
