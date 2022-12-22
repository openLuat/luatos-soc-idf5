#include "luat_base.h"
#include "luat_pm.h"

#include "esp_sleep.h"
#include "driver/rtc_io.h"

#define LUAT_LOG_TAG "pm"
#include "luat_log.h"

#define DEFAULT_WAKEUP_PIN 8

int luat_pm_request(int mode) {
    if (mode == LUAT_PM_SLEEP_MODE_LIGHT) {
        esp_light_sleep_start();
        return 0;
    }
    else if (mode == LUAT_PM_SLEEP_MODE_DEEP || mode == LUAT_PM_SLEEP_MODE_STANDBY) {
#if SOC_GPIO_SUPPORT_DEEPSLEEP_WAKEUP
        const gpio_config_t config = {
            .pin_bit_mask = BIT(DEFAULT_WAKEUP_PIN),
            .mode = GPIO_MODE_INPUT,
        };
        gpio_config(&config);
        esp_deep_sleep_enable_gpio_wakeup(BIT(DEFAULT_WAKEUP_PIN), ESP_GPIO_WAKEUP_GPIO_LOW);
#endif
        esp_deep_sleep_start();
        return 0;
    }
    return -1;
}

int luat_pm_dtimer_start(int id, size_t timeout) {
    return -1;
}

int luat_pm_dtimer_stop(int id) {
    return -1;
}

int luat_pm_dtimer_check(int id) {
    return -1;
}

int luat_pm_last_state(int *lastState, int *rtcOrPad) {
    return 0;
}

int luat_pm_force(int mode) {
    return luat_pm_request(mode);
}

int luat_pm_check(void) {
    return 0;
}

int luat_pm_dtimer_wakeup_id(int* id) {
    return 0;
}

int luat_pm_dtimer_list(size_t* count, size_t* list) {
    return 0;
}

int luat_pm_poweroff(void) {
    LLOGW("powerOff is not supported");
    return -1;
}
