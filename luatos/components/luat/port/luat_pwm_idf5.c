
#include "luat_base.h"
#include "luat_pwm.h"

#include "driver/ledc.h"

#include "luat_log.h"
#define LUAT_LOG_TAG "pwm"

static uint8_t luat_pwm_idf[LEDC_TIMER_MAX] = {0};

int luat_pwm_setup(luat_pwm_conf_t *conf){
    int channel = conf->channel;
    if (channel<0 || channel>=LEDC_TIMER_MAX) {
        return -1;
    }
    if (luat_pwm_idf[channel] == 0){
        ledc_timer_config_t ledc_timer = {
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .duty_resolution = LEDC_TIMER_13_BIT,
            .timer_num = channel,
            .freq_hz = conf->period,
            .clk_cfg = LEDC_AUTO_CLK,
        };
        ledc_timer_config(&ledc_timer);

        ledc_channel_config_t ledc_channel = {
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .channel = channel,
            .timer_sel = channel,
            .intr_type = LEDC_INTR_DISABLE,
            .duty = 0,
            .hpoint = 0,
        };
        switch (channel){
            case 0:
                ledc_channel.gpio_num = 6;
                break;
            case 1:
                ledc_channel.gpio_num = 2;
                break;
            case 2:
                ledc_channel.gpio_num = 10;
                break;
            case 3:
                ledc_channel.gpio_num = 8;
                break;
        }
        ledc_channel_config(&ledc_channel);
        luat_pwm_idf[channel] = 1;
    }
    ledc_set_freq(LEDC_LOW_SPEED_MODE, channel, conf->period);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, 8192 *conf->pulse/100 );
    ledc_update_duty(LEDC_LOW_SPEED_MODE, channel);
    return 0;
}

int luat_pwm_close(int channel){
    if (channel<0 || channel>=LEDC_TIMER_MAX) {
        return -1;
    }
    ledc_stop(LEDC_LOW_SPEED_MODE, channel, 0);
    switch (channel){
        case 0:
            gpio_reset_pin(6);
            break;
        case 1:
            gpio_reset_pin(2);
            break;
        case 2:
            gpio_reset_pin(10);
            break;
        case 3:
            gpio_reset_pin(8);
            break;
    }
    luat_pwm_idf[channel] = 0;
    return 0;
}

int luat_pwm_capture(int channel, int freq){
    return -1;
}
