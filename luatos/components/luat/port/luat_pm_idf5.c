#include "luat_base.h"
#include "luat_pm.h"

#include "esp_sleep.h"
#include "driver/rtc_io.h"

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

#define LUAT_LOG_TAG "pm"
#include "luat_log.h"

int luat_pm_request(int mode) {
    if (mode == LUAT_PM_SLEEP_MODE_LIGHT) {
        esp_sleep_enable_gpio_wakeup();
        esp_light_sleep_start();
        return 0;
    }
    else if (mode == LUAT_PM_SLEEP_MODE_DEEP || mode == LUAT_PM_SLEEP_MODE_STANDBY) {
        esp_deep_sleep_start();
        return 0;
    }
    return -1;
}

int luat_pm_dtimer_start(int id, size_t timeout) {
    esp_sleep_enable_timer_wakeup(timeout * 1000);
    return 0;
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

int luat_pm_power_ctrl(int id, uint8_t onoff) {
    LLOGW("not support yet");
    return -1;
}

int luat_pm_get_poweron_reason(void){
//     switch (esp_sleep_get_wakeup_cause()) {
// #if CONFIG_EXAMPLE_EXT0_WAKEUP
//         case ESP_SLEEP_WAKEUP_EXT0: {
//             printf("Wake up from ext0\n");
//             break;
//         }
// #endif // CONFIG_EXAMPLE_EXT0_WAKEUP
// #ifdef CONFIG_EXAMPLE_EXT1_WAKEUP
//         case ESP_SLEEP_WAKEUP_EXT1: {
//             uint64_t wakeup_pin_mask = esp_sleep_get_ext1_wakeup_status();
//             if (wakeup_pin_mask != 0) {
//                 int pin = __builtin_ffsll(wakeup_pin_mask) - 1;
//                 printf("Wake up from GPIO %d\n", pin);
//             } else {
//                 printf("Wake up from GPIO\n");
//             }
//             break;
//         }
// #endif // CONFIG_EXAMPLE_EXT1_WAKEUP
// #if SOC_GPIO_SUPPORT_DEEPSLEEP_WAKEUP
//         case ESP_SLEEP_WAKEUP_GPIO: {
//             uint64_t wakeup_pin_mask = esp_sleep_get_gpio_wakeup_status();
//             if (wakeup_pin_mask != 0) {
//                 int pin = __builtin_ffsll(wakeup_pin_mask) - 1;
//                 printf("Wake up from GPIO %d\n", pin);
//             } else {
//                 printf("Wake up from GPIO\n");
//             }
//             break;
//         }
// #endif //SOC_GPIO_SUPPORT_DEEPSLEEP_WAKEUP
//         case ESP_SLEEP_WAKEUP_TIMER: {
//             printf("Wake up from timer. Time spent in deep sleep\n");
//             break;
//         }
// #ifdef CONFIG_EXAMPLE_TOUCH_WAKEUP
//         case ESP_SLEEP_WAKEUP_TOUCHPAD: {
//             printf("Wake up from touch on pad %d\n", esp_sleep_get_touchpad_wakeup_status());
//             break;
//         }
// #endif // CONFIG_EXAMPLE_TOUCH_WAKEUP
//         case ESP_SLEEP_WAKEUP_UNDEFINED:
//         default:
//             printf("Not a deep sleep reset\n");
//     }
	return 4;
}

int luat_pm_iovolt_ctrl(int id, int val) {
    return -1;
}

int luat_pm_wakeup_pin(int pin, int val){
    const gpio_config_t config = {
        .pin_bit_mask = BIT64(pin),
        .mode = GPIO_MODE_INPUT,
    };
    gpio_set_pull_mode(pin, val);
    gpio_config(&config);
    gpio_wakeup_enable(pin, val? GPIO_INTR_HIGH_LEVEL : GPIO_INTR_LOW_LEVEL);
#if SOC_GPIO_SUPPORT_DEEPSLEEP_WAKEUP
    esp_deep_sleep_enable_gpio_wakeup(BIT64(pin), val);
#endif
    return 0;
}
