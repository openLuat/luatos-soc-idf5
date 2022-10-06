#include "luat_base.h"
#include "luat_mcu.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_flash.h"

int luat_mcu_set_clk(size_t mhz) {
    return 0;
}
int luat_mcu_get_clk(void) {
    return 160;
}

extern esp_flash_t *esp_flash_default_chip;
static char uid[8];
const char* luat_mcu_unique_id(size_t* t) {
    if (uid[0] == 0) {
        uint64_t tmp;
        int ret = esp_flash_read_unique_chip_id(esp_flash_default_chip, &tmp);
        if (ret == ESP_OK) {
            memcpy(uid, &tmp, sizeof(tmp));
        }
    }
    *t = 8;
    return (const char*)uid;
}

long luat_mcu_ticks(void) {
    return xTaskGetTickCount();
}

uint32_t luat_mcu_hz(void) {
    return configTICK_RATE_HZ;
}

uint64_t luat_mcu_tick64(void) {
    return 0;
}

int luat_mcu_us_period(void) {
    return 0;
}

uint64_t luat_mcu_tick64_ms(void) {
    return 0;
}
void luat_mcu_set_clk_source(uint8_t source_main, uint8_t source_32k, uint32_t delay) {
    // nop
}
