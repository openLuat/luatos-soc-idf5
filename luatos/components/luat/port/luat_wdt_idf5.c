#include "luat_base.h"
#include "luat_wdt.h"

#include "esp_task_wdt.h"

#include "luat_log.h"
#define LUAT_LOG_TAG "wdt"

esp_task_wdt_config_t twdt_config = {
    .timeout_ms = 0,
    .idle_core_mask = 0,
    .trigger_panic = false,
};

int luat_wdt_init(size_t timeout){
    twdt_config.timeout_ms = timeout;
    esp_task_wdt_init(&twdt_config);
    return esp_task_wdt_add(NULL);
}

int luat_wdt_feed(void){
    return esp_task_wdt_reset();
}

int luat_wdt_set_timeout(size_t timeout){
    twdt_config.timeout_ms = timeout;
    return esp_task_wdt_reconfigure(&twdt_config);
}

int luat_wdt_close(void){
    esp_task_wdt_delete(NULL);
    return esp_task_wdt_deinit();
}