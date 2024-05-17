#include "luat_base.h"
#include "luat_mcu.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_flash.h"
#include "driver/gptimer.h"
#include "spi_flash/spi_flash_defs.h"
#include "esp_pm.h"

enum TYPE_FLASH_ID{
	SPIFLASH_MID_GD = 0xC8,
	SPIFLASH_MID_ESMT = 0x1C,
	SPIFLASH_MID_PUYA = 0x85,	
	SPIFLASH_MID_WINBOND = 0xEF,	
	SPIFLASH_MID_FUDANMICRO = 0xA1,
	SPIFLASH_MID_BOYA       = 0x68,
	SPIFLASH_MID_XMC		= 0x20,
	SPIFLASH_MID_XTX        = 0x0B,
	SPIFLASH_MID_TSINGTENG    = 0xEB, /*UNIGROUP TSINGTENG*/	
	SPIFLASH_MID_TSINGTENG_1MB_4MB    = 0xCD, /*UNIGROUP TSINGTENG*/
};

#define LUAT_LOG_TAG "mcu"
#include "luat_log.h"

int luat_mcu_set_clk(size_t mhz) {
    esp_pm_config_t config = {0};
    esp_pm_get_configuration(&config);
    if (config.max_freq_mhz == mhz) {
        return 0;
    }
    config.max_freq_mhz = mhz;
    if (config.min_freq_mhz > config.max_freq_mhz) {
        config.min_freq_mhz = config.max_freq_mhz;
    }
    int ret = esp_pm_configure(&config);
    if (ret) {
        LLOGD("set clk %d ret %d", mhz, ret);
    }
    return ret;
}

int luat_mcu_get_clk(void) {
    esp_pm_config_t config = {0};
    esp_pm_get_configuration(&config);
    return config.max_freq_mhz;
}

extern esp_flash_t *esp_flash_default_chip;
static uint8_t uid[18];
int uni_bytes = 8;
const char* luat_mcu_unique_id(size_t* t) {
    esp_flash_t *chip = esp_flash_default_chip;
    if (uid[0] == 0) {
        // 这里使用新的UNIQUE_ID实现, 原因是部分FLASH的ID是16字节的
        // 而esp_flash_read_unique_chip_id只能读取前8字节,就可能出现重复的情况
        // 添加这个宏是考虑到有客户可能仍旧需要使用老的unique_id
        // 新的unique_id均添加2个字节, 标识芯片id和容量, 与air101系列保持一致
        #ifndef LUAT_MCU_UNIQUE_ID_OLDSTYLE
        uint32_t tmpvalue = 0;
        esp_flash_read_id(chip, &tmpvalue);
        int rid = (tmpvalue >> 16) & 0xFF;
        // LLOGD("chip id %04X %02X", tmpvalue, rid);
        uid[0] = (uint8_t)rid;
        esp_flash_get_physical_size(chip, &tmpvalue);
        // LLOGD("physical_size %04X", tmpvalue);
        uid[1] = (uint8_t)(tmpvalue >> 20);
        int dumy_bytes = 4;
	    switch(rid)
	    {
	    	case SPIFLASH_MID_GD:
	    	case SPIFLASH_MID_PUYA:
	    	case SPIFLASH_MID_TSINGTENG:
	    	case SPIFLASH_MID_TSINGTENG_1MB_4MB:
	    		dumy_bytes = 4;
	    		uni_bytes = 16;
	    		break;
	    	case SPIFLASH_MID_WINBOND:
	    	case SPIFLASH_MID_FUDANMICRO:
	    	case SPIFLASH_MID_BOYA:
	    	case SPIFLASH_MID_XMC:
	    		dumy_bytes = 4;
	    		uni_bytes = 8;
	    		break;
	    	case SPIFLASH_MID_ESMT:
	    	case SPIFLASH_MID_XTX:
	    	default:
	    		break;
	    }
        // LLOGD("query RUID %d %d", uni_bytes, dumy_bytes);
        spi_flash_trans_t transfer = {
            .command = CMD_RDUID,
            .miso_len = uni_bytes,
            .miso_data = ((uint8_t *)uid + 2),
            .dummy_bitlen = dumy_bytes * 8, //RDUID command followed by 4 bytes (32 bits) of dummy clocks.
        };
        esp_err_t err = chip->host->driver->common_command(chip->host, &transfer);
        // LLOGD("ret %d", err);
        if (err == 0) {
            size_t max = uni_bytes;
            for (size_t i = 0; i < max; i++)
            {
                if (uid[max - i + 2] == 0xFF)
                    uni_bytes -= 1;
                else
                    break;
            }
            if (uni_bytes == 0) {
                LLOGE("flash chip NO support unique id");
            }
        }
        #else
        uint64_t chip_uid = 0;
        uint8_t uuid2[8];
        esp_flash_read_unique_chip_id(chip, &chip_uid);
        memcpy(uuid2, &chip_uid, 8);
        // LLOGD("ORG %02X%02X%02X%02X%02X%02X%02X%02X", uuid2[0], uuid2[1], uuid2[2], uuid2[3], 
        //                                               uuid2[4], uuid2[5], uuid2[6], uuid2[7]);
        memcpy(uid, uuid2, 8);
        uni_bytes = 6;
        #endif
    }
    *t = uni_bytes + 2;
    return (const char*)uid;
}

long luat_mcu_ticks(void) {
    return xTaskGetTickCount();
}

uint32_t luat_mcu_hz(void) {
    return configTICK_RATE_HZ;
}

static gptimer_handle_t us_timer;
uint64_t luat_mcu_tick64(void) {
    uint64_t ret = 0;
    gptimer_get_raw_count(us_timer, &ret);
    // LLOGD("tick64 %lld", ret);
    return ret;
}

int luat_mcu_us_period(void) {
    return 1;
}

uint64_t luat_mcu_tick64_ms(void) {
    return luat_mcu_tick64() / 1000;
}
void luat_mcu_set_clk_source(uint8_t source_main, uint8_t source_32k, uint32_t delay) {
    // nop
}

#define TIMER_DIVIDER         (16)  //  Hardware timer clock divider
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds

void luat_mcu_us_timer_init() {
        /* Select and initialize basic parameters of the timer */
    const gptimer_config_t config = {
        #if defined(CONFIG_IDF_TARGET_ESP32)
        .clk_src = GPTIMER_CLK_SRC_APB,
        #else
        .clk_src = GPTIMER_CLK_SRC_XTAL,
        #endif
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000, // us
    }; // default clock source is APB
    int ret = gptimer_new_timer(&config, &us_timer);
    // LLOGD("gptimer_new_timer %d", ret);
    if (ret == 0) {
        if (0 == gptimer_enable(us_timer))
            gptimer_start(us_timer);
    }
}

void luat_mcu_set_hardfault_mode(int mode) {;}
void luat_mcu_xtal_ref_output(uint8_t main_enable, uint8_t slow_32k_enable) {;}
int luat_uart_pre_setup(int uart_id, uint8_t use_alt_type){return -1;}
int luat_i2c_set_iomux(int id, uint8_t value){return -1;}
