#include "luat_base.h"
#include "luat_msgbus.h"
#include "luat_nimble.h"

// #include "esp_log.h"
// /* BLE */
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
// #include "host/ble_hs.h"
// #include "host/util/util.h"
// // #include "console/console.h"
// #include "services/gap/ble_svc_gap.h"
// #include "services/gatt/ble_svc_gatt.h"
// #include "blehr_sens.h"


#define LUAT_LOG_TAG "nimble"
#include "luat_log.h"

void bleprph_host_task(void *param)
{
    LLOGI("BLE Host Task Started");
    /* This function will return only when nimble_port_stop() is executed */
    nimble_port_run();

    nimble_port_freertos_deinit();
}

int luat_nimble_deinit() {
    LLOGE("deinit not support yet");
    return -1;
}

int luat_nimble_trace_level(int level) {
    return 0;
}

int luat_nimble_init_peripheral(uint8_t uart_idx, char* name, int mode);
int luat_nimble_init_central(uint8_t uart_idx, char* name, int mode);

int luat_nimble_init(uint8_t uart_idx, char* name, int mode) {
    int ret = -1;
    if (mode == 0) {
        ret = luat_nimble_init_peripheral(uart_idx, name, mode);
    }
    else if (mode == 1) {
        ret = luat_nimble_init_central(uart_idx, name, mode);
    }
    if (ret == 0) {
        nimble_port_freertos_init(bleprph_host_task);
    }
    return ret;
}
