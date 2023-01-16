
#include "luat_base.h"
#include "luat_rtos.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#define LUAT_LOG_TAG "rtos"
#include "luat_log.h"

uint32_t luat_rtos_get_ipsr(void){
    return xPortInIsrContext();
}
