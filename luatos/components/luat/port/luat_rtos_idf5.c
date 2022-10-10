
#include "luat_base.h"
#include "luat_rtos.h"
#include "luat_malloc.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#define LUAT_LOG_TAG "rtos"
#include "luat_log.h"

typedef struct luat_rtos_timer {
    TimerHandle_t timer;
	void *cb;
	void *param;
}luat_rtos_timer_t;

void *luat_create_rtos_timer(void *cb, void *param, void *task_handle){
	luat_rtos_timer_t *luat_timer = luat_heap_malloc(sizeof(luat_rtos_timer_t));
	luat_timer->cb = cb;
	luat_timer->param = param;
	return luat_timer;
}

int luat_start_rtos_timer(void *timer, uint32_t ms, uint8_t is_repeat){
	luat_rtos_timer_t *luat_timer = (luat_rtos_timer_t *)timer;
	luat_timer->timer = xTimerCreate(NULL, ms,
                                 is_repeat, NULL, luat_timer->cb);
	xTimerStart(luat_timer->timer, 5);
	return 0;
}

void luat_stop_rtos_timer(void *timer){
	luat_rtos_timer_t *luat_timer = (luat_rtos_timer_t *)timer;
	xTimerStop(luat_timer->timer, 1);
}
void luat_release_rtos_timer(void *timer){
	luat_rtos_timer_t *luat_timer = (luat_rtos_timer_t *)timer;
	xTimerStop(luat_timer->timer, 1);
	xTimerDelete(luat_timer->timer, 1);
	luat_heap_free(luat_timer);
}
