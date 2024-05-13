#include "luat_base.h"
#include "luat_wdt.h"

#include "esp_task_wdt.h"

#include "luat_log.h"
#define LUAT_LOG_TAG "wdt"

static uint32_t task_added = 0;

int luat_wdt_init(size_t timeout){
    if (task_added != 0)
        return 0;
    luat_wdt_set_timeout(timeout);
    esp_task_wdt_add(NULL);
    task_added = 1;
    return 0;
}

int luat_wdt_feed(void){
    return esp_task_wdt_reset();
}

int luat_wdt_set_timeout(size_t timeout){
    esp_task_wdt_config_t twdt_config = {
        .timeout_ms = timeout,
        .idle_core_mask = 0,
        .trigger_panic = true,
    };
    esp_task_wdt_reconfigure(&twdt_config);
    return 0;
}

int luat_wdt_close(void){
    esp_task_wdt_delete(NULL);
    return esp_task_wdt_deinit();
}