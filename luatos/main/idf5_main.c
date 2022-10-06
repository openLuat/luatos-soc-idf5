
#include <string.h>
#include <stdlib.h>
#include "esp_event.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_log.h"


extern int luat_main (void);
extern void bootloader_random_enable(void);
extern void luat_heap_init(void);

void app_main(void)
{
    bootloader_random_enable();
    esp_err_t r = nvs_flash_init();
    if (r == ESP_ERR_NVS_NO_FREE_PAGES || r == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        //ESP_LOGI("no free pages or nvs version mismatch, erase...");
        nvs_flash_erase();
        r = nvs_flash_init();
    }
    luat_heap_init();
    esp_event_loop_create_default();
    luat_main();
}
