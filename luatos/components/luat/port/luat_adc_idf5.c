
#include "luat_base.h"
#include "luat_adc.h"

#include "driver/adc.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#include "driver/temperature_sensor.h"

#include "luat_log.h"
#define LUAT_LOG_TAG "adc"

#define ADC_CHECK(id) ((id<0||id>=ADC1_CHANNEL_MAX)?-1:0)

static adc_oneshot_unit_handle_t adc1_handle = NULL;
static adc_cali_handle_t adc1_cali_handle = NULL;

static uint8_t adc_init = 0;
static temperature_sensor_handle_t temp_sensor = NULL;

int luat_adc_open(int pin, void *args){
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_11,
    };
    if (ADC_CHECK(pin)){
        if (pin==LUAT_ADC_CH_CPU && temp_sensor == NULL){
#if defined(CONFIG_IDF_TARGET_ESP32C2)||defined(CONFIG_IDF_TARGET_ESP32C3)||defined(CONFIG_IDF_TARGET_ESP32S2)||defined(CONFIG_IDF_TARGET_ESP32S3)
            temperature_sensor_config_t temp_sensor_config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(-10, 80);
            temperature_sensor_install(&temp_sensor_config, &temp_sensor);
            temperature_sensor_enable(temp_sensor);
            return 0;
#endif
        }
        return -1;
    }
    if (adc_init==0){
        adc_oneshot_unit_init_cfg_t init_config1 = {
            .unit_id = ADC_UNIT_1,
        };
        adc_oneshot_new_unit(&init_config1, &adc1_handle);
        adc_oneshot_config_channel(adc1_handle, pin, &config);
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = ADC_UNIT_1,
            .atten = ADC_ATTEN_DB_11,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        adc_cali_create_scheme_curve_fitting(&cali_config, &adc1_cali_handle);
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = ADC_UNIT_1,
            .atten = ADC_ATTEN_DB_11,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        adc_cali_create_scheme_line_fitting(&cali_config, &adc1_cali_handle);
#endif
        adc_init = 1;
    }else{
        adc_oneshot_config_channel(adc1_handle, pin, &config);
    }
    return 0;
}

int luat_adc_read(int pin, int *val, int *val2){
    if (ADC_CHECK(pin)){
        if (pin == LUAT_ADC_CH_CPU){
#if defined(CONFIG_IDF_TARGET_ESP32C2)||defined(CONFIG_IDF_TARGET_ESP32C3)||defined(CONFIG_IDF_TARGET_ESP32S2)||defined(CONFIG_IDF_TARGET_ESP32S3)
            float tsens_value;
            temperature_sensor_get_celsius(temp_sensor, &tsens_value);
            *val = (int)(tsens_value*1000);
            *val2 = (int)(tsens_value*1000);
            return 0;
#endif
        }
        return -1;
    }
    adc_oneshot_read(adc1_handle, pin, val);
    adc_cali_raw_to_voltage(adc1_cali_handle, *val, val2);
    return 0;
}

int luat_adc_close(int pin){
    if (ADC_CHECK(pin)){
        if (pin == LUAT_ADC_CH_CPU){
#if defined(CONFIG_IDF_TARGET_ESP32C2)||defined(CONFIG_IDF_TARGET_ESP32C3)||defined(CONFIG_IDF_TARGET_ESP32S2)||defined(CONFIG_IDF_TARGET_ESP32S3)
            temperature_sensor_disable(temp_sensor);
            temperature_sensor_uninstall(temp_sensor);
            temp_sensor = NULL;
            return 0;
#endif
        }
        return -1;
    }
    gpio_reset_pin(pin);
    return 0;
}

int luat_adc_global_config(int tp, int val) {
    return -1; // 暂时不支持配置, 放个空的函数在这里
}
