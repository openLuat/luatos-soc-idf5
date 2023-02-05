
#include "luat_base.h"
#include "luat_gpio.h"
#include "luat_irq.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

#define LUAT_LOG_TAG "gpio"
#include "luat_log.h"

#define GPIO_CHECK(pin) ((pin<0 || pin>=SOC_GPIO_PIN_COUNT) ? -1:0)

#if defined(CONFIG_IDF_TARGET_ESP32C3)
static uint8_t warn_c3_gpio1819 = 0;
#endif

typedef struct gpio_cb_args
{
    luat_gpio_irq_cb irq_cb;
    void*irq_args;
}gpio_cb_args_t;

static gpio_cb_args_t gpio_isr_cb[SOC_GPIO_PIN_COUNT] = {0};

static uint8_t gpio_isr_sta = 0;

int luat_gpio_irq_default(int pin, void* args);

static void IRAM_ATTR gpio_isr_handler(void *arg) {
    uint32_t pin = (uint32_t)arg;
    if (gpio_isr_cb[pin].irq_cb)
    {
        gpio_isr_cb[pin].irq_cb(pin,gpio_isr_cb[pin].irq_args);
    }
    else
        luat_gpio_irq_default(pin, (void*)luat_gpio_get(pin));
}

int luat_gpio_setup(luat_gpio_t *gpio) {
    if (GPIO_CHECK(gpio->pin)) {
        LLOGW("not such GPIO number %d", gpio->pin);
        return -1;
    }
    int pin = gpio->pin;

#if defined(CONFIG_IDF_TARGET_ESP32C3)
    // 如果是GPIO18/19, 这两个2脚也用在USB, 在简约版会导致USB连接中断
    if ((18 == pin || 19 == pin) && warn_c3_gpio1819 == 0) {
        if (warn_c3_gpio1819 == 0) {
            warn_c3_gpio1819 = 1;
            LLOGI("GPIO 18/19 is use for USB too");
            gpio_reset_pin(18);
            gpio_reset_pin(19);
        }
    }
#elif defined(CONFIG_IDF_TARGET_ESP32S3)

#endif

    // 重置管脚的功能为GPIO
    gpio_reset_pin(pin);

    // 设置输出/输入/中断状态
    if (gpio->mode == Luat_GPIO_OUTPUT) {
        // 输出模式
        gpio_set_direction(pin, GPIO_MODE_OUTPUT);
        // 还得设置初始状态
        gpio_set_level(pin, gpio->irq); // 输出模式时, gpio-irq兼职输出状态值
    }
    else if (gpio->mode == Luat_GPIO_INPUT) {
        gpio_set_direction(pin, GPIO_MODE_INPUT);
    }
    else if (gpio->mode == Luat_GPIO_IRQ) {
        if (gpio_isr_sta == 0) {
            gpio_install_isr_service(0);
            gpio_isr_sta = 1;
        }
        // 中断当然是输入模式了
        gpio_set_direction(pin, GPIO_MODE_INPUT);
        switch (gpio->irq) {
        // 上升沿
        case Luat_GPIO_RISING:
            gpio_set_intr_type(pin, GPIO_INTR_POSEDGE);
            break;
        // 下降沿
        case Luat_GPIO_FALLING:
            gpio_set_intr_type(pin, GPIO_INTR_NEGEDGE);
            break;
        // 高电平
        case Luat_GPIO_HIGH_IRQ:
            gpio_set_intr_type(pin, GPIO_INTR_HIGH_LEVEL);
            break;
        // 低电平
        case Luat_GPIO_LOW_IRQ:
            gpio_set_intr_type(pin, GPIO_INTR_LOW_LEVEL);
            break;
        // 双向触发, 上升沿和下降沿都触发
        case Luat_GPIO_BOTH:
        default:
            gpio_set_intr_type(pin, GPIO_INTR_ANYEDGE);
            break;
        }
        if (gpio->irq_cb) {
            gpio_isr_cb[pin].irq_cb = gpio->irq_cb;
            gpio_isr_cb[pin].irq_args = gpio->irq_args;
        }
        gpio_isr_handler_add(pin, gpio_isr_handler, (void *)pin);
    }
    // 上拉/下拉状态
    switch (gpio->pull) {
    case Luat_GPIO_PULLUP:
        gpio_set_pull_mode(pin, GPIO_PULLUP_ONLY);
        break;
    case Luat_GPIO_PULLDOWN:
        gpio_set_pull_mode(pin, GPIO_PULLDOWN_ONLY);
        break;
    // 默认浮空状态
    case Luat_GPIO_DEFAULT:
    default:
        gpio_set_pull_mode(pin, GPIO_FLOATING);
        break;
    }
    return 0;
}

int luat_gpio_set(int pin, int level) {
    if (pin == Luat_GPIO_MAX_ID)
        return 0;
    if (GPIO_CHECK(pin)) {
        return -1;
    }
    gpio_set_level(pin, level);
    return 0; // 总是返回正常, pin正确不可能失败吧
}

int luat_gpio_get(int pin) {
    if (pin == Luat_GPIO_MAX_ID)
        return 0;
    if (GPIO_CHECK(pin)) {
        return 0; // 不存在? 总是返回0, 低电平
    }
    return gpio_get_level(pin);
}

void luat_gpio_close(int pin) {
    if (GPIO_CHECK(pin)) {
        return;
    }
    if (gpio_isr_cb[pin].irq_cb)
    {
        gpio_isr_cb[pin].irq_cb = 0;
        gpio_isr_cb[pin].irq_args = 0;
    }
    gpio_reset_pin(pin);
}

#include "soc/gpio_reg.h"
#define GPIO_OUT_PULSE	(*(volatile unsigned int*)(GPIO_OUT_REG))

static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR luat_gpio_pulse(int pin, uint8_t *level, uint16_t len, uint16_t delay_ns) {
    volatile uint32_t tmp = delay_ns > 1024 ? 1024 : delay_ns;
    portENTER_CRITICAL_SAFE(&mux);
    for(int i=0; i<len; i++) {
        if(level[i/8]&(0x80>>(i%8)))
            GPIO_OUT_PULSE |= (0x00000001<<pin); // TODO 预先计算会不会更好一些, 也更快
        else 
            GPIO_OUT_PULSE &= ~(0x00000001<<pin);
        tmp = delay_ns > 1024 ? 1024 : delay_ns;
        while(tmp--); // nop
    }
    portEXIT_CRITICAL_SAFE(&mux);
}
