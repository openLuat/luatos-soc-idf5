
#include "luat_base.h"
#include "luat_pwm.h"

#include <math.h>

#include "driver/ledc.h"

#include "luat_log.h"
#define LUAT_LOG_TAG "pwm"

static uint8_t luat_pwm_idf[LEDC_TIMER_MAX] = {0};

int luat_pwm_setup(luat_pwm_conf_t *conf){
    int resolution = 0;
    int timer = -1;
    int ret = 0;
    for (size_t i = 0; i < LEDC_TIMER_MAX; i++){
        if (luat_pwm_idf[i]==0 || luat_pwm_idf[i]==conf->channel+1){
            timer = i;
        }
    }
    if (timer < 0) {
        LLOGE("too many PWM!!! only %d channels supported", LEDC_TIMER_MAX);
        return -1;
    }
    resolution = ceil(log2(conf->precision));
    if (luat_pwm_idf[timer] == 0){
        ledc_timer_config_t ledc_timer = {
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .timer_num = timer,
            .freq_hz = conf->period,
            .clk_cfg = LEDC_AUTO_CLK,
        };
        ledc_timer.duty_resolution = resolution;

        ret = ledc_timer_config(&ledc_timer);
        if (ret) {
            return -1;
        }

        ledc_channel_config_t ledc_channel = {
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .channel = timer,
            .timer_sel = timer,
            .intr_type = LEDC_INTR_DISABLE,
            .gpio_num = conf->channel,
            .duty = 0,
            .hpoint = 0,
        };
        ledc_channel_config(&ledc_channel);
        luat_pwm_idf[timer] = conf->channel+1;//避免使用0
    }
    ledc_set_freq(LEDC_LOW_SPEED_MODE, timer, conf->period);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, timer, pow(2, resolution) * conf->pulse / conf->precision );
    ledc_update_duty(LEDC_LOW_SPEED_MODE, timer);
    return 0;
}

int luat_pwm_close(int channel){
    int timer = -1;
    for (size_t i = 0; i < LEDC_TIMER_MAX; i++){
        if (luat_pwm_idf[i]==channel+1){
            timer = i;
        }
    }
    if (channel < 0 || channel>=LEDC_TIMER_MAX || timer<0) {
        return -1;
    }
    ledc_stop(LEDC_LOW_SPEED_MODE, timer, 0);
    gpio_reset_pin(0);
    luat_pwm_idf[timer] = 0;
    return 0;
}

int luat_pwm_capture(int channel, int freq){
    return -1;
}
