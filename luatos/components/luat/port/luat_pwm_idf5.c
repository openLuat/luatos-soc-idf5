
#include "luat_base.h"
#include "luat_pwm.h"

#include "driver/ledc.h"

#include "luat_log.h"
#define LUAT_LOG_TAG "pwm"

static uint8_t luat_pwm_idf[LEDC_TIMER_MAX] = {0};

int luat_pwm_setup(luat_pwm_conf_t *conf){
    int channel;
    if (conf->channel > 9){
        channel = conf->channel%10;
    }else{
        channel = conf->channel;
    }
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
        switch (conf->channel){
            case 00:
                ledc_channel.gpio_num = 0;
                break;
            case 01:
                ledc_channel.gpio_num = 1;
                break;
            case 02:
                ledc_channel.gpio_num = 2;
                break;
            case 03:
                ledc_channel.gpio_num = 3;
                break;
            case 10:
                ledc_channel.gpio_num = 4;
                break;
            case 11:
                ledc_channel.gpio_num = 5;
                break;
            case 12:
                ledc_channel.gpio_num = 6;
                break;
            case 13:
                ledc_channel.gpio_num = 7;
                break;
            case 20:
                ledc_channel.gpio_num = 8;
                break;
            case 21:
                ledc_channel.gpio_num = 9;
                break;
            case 22:
                ledc_channel.gpio_num = 10;
                break;
            case 23:
                ledc_channel.gpio_num = 11;
                break;
            case 30:
                ledc_channel.gpio_num = 12;
                break;
            case 31:
                ledc_channel.gpio_num = 13;
                break;
            case 32:
                ledc_channel.gpio_num = 14;
                break;
            case 33:
                ledc_channel.gpio_num = 15;
                break;
            case 40:
                ledc_channel.gpio_num = 16;
                break;
            case 41:
                ledc_channel.gpio_num = 17;
                break;
            case 42:
                ledc_channel.gpio_num = 18;
                break;
            case 43:
                ledc_channel.gpio_num = 19;
                break;
            case 50:
                ledc_channel.gpio_num = 20;
                break;
            case 51:
                ledc_channel.gpio_num = 21;
                break;
            default:
                LLOGW("unkown pwm channel %d", channel);
                return -1;
        }
        ledc_channel_config(&ledc_channel);
        luat_pwm_idf[channel] = 1;
    }
    ledc_set_freq(LEDC_LOW_SPEED_MODE, channel, conf->period);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, 8192 * conf->pulse / conf->precision );
    ledc_update_duty(LEDC_LOW_SPEED_MODE, channel);
    return 0;
}

int luat_pwm_close(int channel){
    int timer;
    if (channel > 9){
        timer = channel%10;
    }else{
        timer = channel;
    }
    if (timer<0 || timer>=LEDC_TIMER_MAX) {
        return -1;
    }
    ledc_stop(LEDC_LOW_SPEED_MODE, timer, 0);
    switch (channel){
        case 00:
            gpio_reset_pin(0);
            break;
        case 01:
            gpio_reset_pin(0);
            break;
        case 02:
            gpio_reset_pin(0);
            break;
        case 03:
            gpio_reset_pin(0);
            break;
        case 10:
            gpio_reset_pin(0);
            break;
        case 11:
            gpio_reset_pin(0);
            break;
        case 12:
            gpio_reset_pin(0);
            break;
        case 13:
            gpio_reset_pin(0);
            break;
        case 20:
            gpio_reset_pin(0);
            break;
        case 21:
            gpio_reset_pin(0);
            break;
        case 22:
            gpio_reset_pin(0);
            break;
        case 23:
            gpio_reset_pin(0);
            break;
        case 30:
            gpio_reset_pin(0);
            break;
        case 31:
            gpio_reset_pin(0);
            break;
        case 32:
            gpio_reset_pin(0);
            break;
        case 33:
            gpio_reset_pin(0);
            break;
        case 40:
            gpio_reset_pin(0);
            break;
        case 41:
            gpio_reset_pin(0);
            break;
        case 42:
            gpio_reset_pin(0);
            break;
        case 43:
            gpio_reset_pin(0);
            break;
        case 50:
            gpio_reset_pin(0);
            break;
        case 51:
            gpio_reset_pin(0);
            break;
        default:
            LLOGW("unkown pwm channel %d", channel);
            return -1;
    }
    luat_pwm_idf[timer] = 0;
    return 0;
}

int luat_pwm_capture(int channel, int freq){
    return -1;
}
