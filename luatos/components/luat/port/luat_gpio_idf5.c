
#include "luat_base.h"
#include "luat_gpio.h"
#include "luat_irq.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

#define LUAT_LOG_TAG "luat.gpio"
#include "luat_log.h"


static void IRAM_ATTR gpio_isr_handler(void *arg){
    uint32_t pin = (uint32_t)arg;
    luat_gpio_irq_default(pin, (void*)luat_gpio_get(pin));
}

int luat_gpio_setup(luat_gpio_t *gpio){
    static uint8_t gpio_isr_sta = 0;
    gpio_reset_pin(gpio->pin);
    if (gpio->mode == Luat_GPIO_OUTPUT){
        gpio_set_direction(gpio->pin, GPIO_MODE_OUTPUT);
    }
    else if (gpio->mode == Luat_GPIO_INPUT){
        gpio_set_direction(gpio->pin, GPIO_MODE_INPUT);
    }
    else if (gpio->mode == Luat_GPIO_IRQ){
        if (gpio_isr_sta == 0){
            gpio_install_isr_service(0);
            gpio_isr_sta = 1;
        }
        gpio_set_direction(gpio->pin, GPIO_MODE_INPUT);
        switch (gpio->irq){
        case Luat_GPIO_RISING:
            gpio_set_intr_type(gpio->pin, GPIO_INTR_POSEDGE);
            break;
        case Luat_GPIO_FALLING:
            gpio_set_intr_type(gpio->pin, GPIO_INTR_NEGEDGE);
            break;
        case Luat_GPIO_BOTH:
        default:
            gpio_set_intr_type(gpio->pin, GPIO_INTR_ANYEDGE);
            break;
        }
        gpio_isr_handler_add(gpio->pin, gpio_isr_handler, (void *)gpio->pin);
    }
    switch (gpio->pull){
    case Luat_GPIO_DEFAULT:
        gpio_set_pull_mode(gpio->pin, GPIO_FLOATING);
        break;
    case Luat_GPIO_PULLUP:
        gpio_set_pull_mode(gpio->pin, GPIO_PULLUP_ONLY);
        break;
    case Luat_GPIO_PULLDOWN:
        gpio_set_pull_mode(gpio->pin, GPIO_PULLDOWN_ONLY);
        break;
    default:
        break;
    }
    return 0;
}

int luat_gpio_set(int pin, int level){
    return gpio_set_level(pin, level);
}

int luat_gpio_get(int pin){
    return gpio_get_level(pin);
}

void luat_gpio_close(int pin){
    gpio_reset_pin(pin);
}

#include "soc/gpio_reg.h"
#define GPIO_OUT_DATA	(*(volatile unsigned int*)(GPIO_OUT_REG))

void IRAM_ATTR luat_gpio_pulse(int pin, uint8_t *level, uint16_t len, uint16_t delay_ns) {
    volatile uint32_t del=delay_ns;
    vPortEnterCritical();
    for(int i=0; i<len; i++){
        if(level[i/8]&(0x80>>(i%8)))
            GPIO_OUT_DATA |= (0x00000001<<pin);
        else 
            GPIO_OUT_DATA &= ~(0x00000001<<pin);
        del = delay_ns;
        while(del--);
    }
    vPortExitCritical();
}
