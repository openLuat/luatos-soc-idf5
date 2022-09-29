#include "luat_base.h"
#include "luat_mcu.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

int luat_mcu_set_clk(size_t mhz) {
    return 0;
}
int luat_mcu_get_clk(void) {
    return 160;
}

const char* luat_mcu_unique_id(size_t* t);

long luat_mcu_ticks(void) {
    return xTaskGetTickCount();
}

uint32_t luat_mcu_hz(void) {
    return configTICK_RATE_HZ;
}

uint64_t luat_mcu_tick64(void);
int luat_mcu_us_period(void);
uint64_t luat_mcu_tick64_ms(void);
void luat_mcu_set_clk_source(uint8_t source_main, uint8_t source_32k, uint32_t delay) {
    // nop
}
