#include "luat_base.h"
#include "luat_mcu.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_flash.h"
#include "driver/gptimer.h"

#define LUAT_LOG_TAG "mcu"
#include "luat_log.h"

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

static gptimer_handle_t us_timer;
uint64_t luat_mcu_tick64(void) {
    uint64_t ret = 0;
    gptimer_get_raw_count(us_timer, &ret);
    // LLOGD("tick64 %lld", ret);
    return ret;
}

int luat_mcu_us_period(void) {
    return 1;
}

uint64_t luat_mcu_tick64_ms(void) {
    return luat_mcu_tick64() / 1000;
}
void luat_mcu_set_clk_source(uint8_t source_main, uint8_t source_32k, uint32_t delay) {
    // nop
}

#define TIMER_DIVIDER         (16)  //  Hardware timer clock divider
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds

void luat_mcu_us_timer_init() {
        /* Select and initialize basic parameters of the timer */
    const gptimer_config_t config = {
        #ifdef GPTIMER_CLK_SRC_XTAL
        .clk_src = GPTIMER_CLK_SRC_XTAL,
        #else
        .clk_src = GPTIMER_CLK_SRC_APB,
        #endif
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000,
    }; // default clock source is APB
    int ret = gptimer_new_timer(&config, &us_timer);
    // LLOGD("gptimer_new_timer %d", ret);
    if (ret == 0) {
        if (0 == gptimer_enable(us_timer))
            gptimer_start(us_timer);
    }
}