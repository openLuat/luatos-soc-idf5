
#include "luat_base.h"
#include "luat_pwm.h"

#include <math.h>

#include "driver/ledc.h"

#include "luat_log.h"
#define LUAT_LOG_TAG "pwm"

typedef struct pwm_cont {
    luat_pwm_conf_t pc;
    uint8_t is_opened;
}pwm_conf_t;

static pwm_conf_t luat_pwm_idf[LEDC_TIMER_MAX];

#define LEDC_LL_FRACTIONAL_BITS (8)
#define LEDC_LL_FRACTIONAL_MAX     ((1 << LEDC_LL_FRACTIONAL_BITS) - 1)
#define LEDC_TIMER_DIV_NUM_MAX    (0x3FFFF)
#define LEDC_IS_DIV_INVALID(div)  ((div) <= LEDC_LL_FRACTIONAL_MAX || (div) > LEDC_TIMER_DIV_NUM_MAX)

static inline uint32_t ilog2(uint32_t i)
{
    assert(i > 0);
    uint32_t log = 0;
    while (i >>= 1) {
        ++log;
    }
    return log;
}

static inline uint32_t ledc_calculate_divisor(uint32_t src_clk_freq, int freq_hz, uint32_t precision)
{
    uint64_t fp = freq_hz;
    fp *= precision;
    return ( ( (uint64_t) src_clk_freq << LEDC_LL_FRACTIONAL_BITS ) + (fp / 2 ) ) / fp;
}

// https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf#ledpwm
static uint32_t ledc_find_suitable_duty_resolution(uint32_t src_clk_freq, uint32_t timer_freq)
{
    uint32_t div = (src_clk_freq + timer_freq / 2) / timer_freq; // rounded
    uint32_t duty_resolution = ilog2(div);
    if ( duty_resolution >= SOC_LEDC_TIMER_BIT_WIDTH) {
        duty_resolution = SOC_LEDC_TIMER_BIT_WIDTH - 1;
    }
    uint32_t div_param = ledc_calculate_divisor(src_clk_freq, timer_freq, 1 << duty_resolution);
    if (LEDC_IS_DIV_INVALID(div_param)) {
        div = src_clk_freq / timer_freq; // truncated
        duty_resolution = ilog2(div);
        if ( duty_resolution > SOC_LEDC_TIMER_BIT_WIDTH - 1) {
            duty_resolution = SOC_LEDC_TIMER_BIT_WIDTH - 1;
        }
        div_param = ledc_calculate_divisor(src_clk_freq, timer_freq, 1 << duty_resolution);
        if (LEDC_IS_DIV_INVALID(div_param)) {
            duty_resolution = 0;
        }
    }
    return duty_resolution;
}

int luat_pwm_setup(luat_pwm_conf_t *conf){
    int duty_resolution = 0;
    int timer = -1;
    int ret = -1;
    if (conf->channel < 0)
        return -1;
    for (size_t i = 0; i < LEDC_TIMER_MAX; i++)
    {
        if (luat_pwm_idf[i].is_opened && luat_pwm_idf[i].pc.channel == conf->channel) {
            timer = i;
            break;
        }
        if (!luat_pwm_idf[i].is_opened) {
            timer = i;
            break;
        }
    }
    if (timer < 0) {
        LLOGE("too many PWM!!! only %d channels supported", LEDC_TIMER_MAX);
        return -1;
    }
    if (conf->pulse > conf->precision) {
        conf->pulse = conf->precision;
    }


    duty_resolution = ledc_find_suitable_duty_resolution(80*1000*1000, conf->period);
    
    int duty = (conf->pulse * (1 << duty_resolution)) / conf->precision;
    // LLOGD("freq=%d, pulse=%d, precision=%d, resolution=%08X, duty=%08X", (int)conf->period, (int)conf->pulse, (int)conf->precision, (int)(1 << duty_resolution), (int)duty);

    int speed_mode = LEDC_LOW_SPEED_MODE;
    
    // 判断一下是否需要完全重新配置
    if (conf->period != luat_pwm_idf[timer].pc.period || // 频率是否相同
         conf->precision != luat_pwm_idf[timer].pc.precision || // 占空比精度是否相同
         conf->pnum != luat_pwm_idf[timer].pc.pnum // 输出脉冲数是否相同
        ) {
        // LLOGD("need to reconfig channel %d period %d", conf->channel, conf->period);
        ledc_timer_config_t ledc_timer = {
            .speed_mode = speed_mode,
            .timer_num = timer,
            .freq_hz = conf->period,
            .clk_cfg = LEDC_AUTO_CLK,
            .duty_resolution = duty_resolution
        };
        // LLOGD("duty_resolution %d %04X", duty_resolution, 1 << duty_resolution);
        ret = ledc_timer_config(&ledc_timer);
        if (ret) {
            return -1;
        }

        ledc_channel_config_t ledc_channel = {
            .speed_mode = speed_mode,
            .channel = timer,
            .timer_sel = timer,
            .intr_type = LEDC_INTR_DISABLE,
            .gpio_num = conf->channel,
            .duty = duty,
            .hpoint = 0,
        };
        ledc_channel_config(&ledc_channel);
    }
    ledc_set_duty(speed_mode, timer, duty);
    ledc_update_duty(speed_mode, timer);
    memcpy( &luat_pwm_idf[timer].pc, conf, sizeof(luat_pwm_conf_t));
    luat_pwm_idf[timer].is_opened = 1;
    return 0;
}

int luat_pwm_close(int channel){
    int timer = -1;
    if (channel < 0)
        return -1;
    for (size_t i = 0; i < LEDC_TIMER_MAX; i++){
        if (luat_pwm_idf[i].is_opened && luat_pwm_idf[i].pc.channel == channel){
            timer = i;
            break;
        }
    }
    if (timer < 0) {
        return -1;
    }
    int ret = ledc_stop(LEDC_LOW_SPEED_MODE, timer, 0);
    if (ret) {
        return -1;
    }
    gpio_reset_pin(channel);
    luat_pwm_idf[timer].is_opened = 0;
    memset(&luat_pwm_idf[timer].pc, 0, sizeof(luat_pwm_conf_t) );
    return 0;
}

int luat_pwm_capture(int channel, int freq){
    return -1;
}
