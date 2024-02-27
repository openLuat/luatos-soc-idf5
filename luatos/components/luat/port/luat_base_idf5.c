#include "luat_base.h"
#include "luat_msgbus.h"
#include "luat_fs.h"
#include "luat_timer.h"
#include <stdlib.h>

#ifdef LUAT_USE_LVGL
#include "lvgl.h"
void luat_lv_fs_init(void);
void lv_bmp_init(void);
void lv_png_init(void);
void lv_split_jpeg_init(void);
#endif

// C2需要强制关闭全部UI组件,否则2M的flash放不下
#if defined(CONFIG_IDF_TARGET_ESP32C2)
#undef LUAT_USE_LCD
#undef LUAT_USE_LVGL
#undef LUAT_USE_EINK
#undef LUAT_USE_U8G2
#undef LUAT_USE_DISP
#undef LUAT_USE_FONTS
#undef LUAT_USE_GTFONT
#undef LUAT_USE_RSA
#endif

LUAMOD_API int luaopen_nimble( lua_State *L );

static const luaL_Reg loadedlibs[] = {
  {"_G", luaopen_base}, // _G
  {LUA_LOADLIBNAME, luaopen_package}, // require
  {LUA_COLIBNAME, luaopen_coroutine}, // coroutine协程库
  {LUA_TABLIBNAME, luaopen_table},    // table库,操作table类型的数据结构
  {LUA_IOLIBNAME, luaopen_io},        // io库,操作文件
  {LUA_OSLIBNAME, luaopen_os},        // os库,已精简
  {LUA_STRLIBNAME, luaopen_string},   // string库,字符串操作
  {LUA_MATHLIBNAME, luaopen_math},    // math 数值计算
  {LUA_UTF8LIBNAME, luaopen_utf8},
  {LUA_DBLIBNAME, luaopen_debug},     // debug库,已精简
#ifdef LUAT_USE_DBG
#ifndef LUAT_USE_SHELL
#define LUAT_USE_SHELL
#endif
  {"dbg",  luaopen_dbg},               // 调试库
#endif
#if defined(LUA_COMPAT_BITLIB)
  {LUA_BITLIBNAME, luaopen_bit32},    // 不太可能启用
#endif
// 往下是LuatOS定制的库, 如需精简请仔细测试
//----------------------------------------------------------------------
// 核心支撑库, 不可禁用!!
  {"rtos",    luaopen_rtos},              // rtos底层库, 核心功能是队列和定时器
  {"log",     luaopen_log},               // 日志库
  {"timer",   luaopen_timer},             // 延时库
//-----------------------------------------------------------------------
// 设备驱动类, 可按实际情况删减. 即使最精简的固件, 也强烈建议保留uart库
#ifdef LUAT_USE_UART
  {"uart",    luaopen_uart},              // 串口操作
#endif
#ifdef LUAT_USE_GPIO
  {"gpio",    luaopen_gpio},              // GPIO脚的操作
#endif
#ifdef LUAT_USE_I2C
  {"i2c",     luaopen_i2c},               // I2C操作
#endif
#ifdef LUAT_USE_SPI
  {"spi",     luaopen_spi},               // SPI操作
#endif
#ifdef LUAT_USE_ADC
  {"adc",     luaopen_adc},               // ADC模块
#endif
#ifdef LUAT_USE_SDIO
  {"sdio",     luaopen_sdio},             // SDIO模块
#endif
#ifdef LUAT_USE_PWM
  {"pwm",     luaopen_pwm},               // PWM模块
#endif
#ifdef LUAT_USE_WDT
  {"wdt",     luaopen_wdt},               // watchdog模块
#endif
#ifdef LUAT_USE_PM
  {"pm",      luaopen_pm},                // 电源管理模块
#endif
#ifdef LUAT_USE_MCU
  {"mcu",     luaopen_mcu},               // MCU特有的一些操作
#endif
#ifdef LUAT_USE_HWTIMER
  {"hwtimer", luaopen_hwtimer},           // 硬件定时器
#endif
#ifdef LUAT_USE_RTC
  {"rtc", luaopen_rtc},                   // 实时时钟
#endif
#ifdef LUAT_USE_OTP
  // {"otp", luaopen_otp},                   // OTP
#endif
#ifdef LUAT_USE_TOUCHKEY
  {"touchkey", luaopen_touchkey},              // OTP
#endif
  // {"pin", luaopen_pin},                   // pin
//-----------------------------------------------------------------------
// 工具库, 按需选用
#ifdef LUAT_USE_CRYPTO
  {"crypto",luaopen_crypto},            // 加密和hash模块
#endif
#ifdef LUAT_USE_CJSON
  {"json",    luaopen_cjson},          // json的序列化和反序列化
#endif
#ifdef LUAT_USE_ZBUFF
  {"zbuff",   luaopen_zbuff},             // 像C语言语言操作内存块
#endif
#ifdef LUAT_USE_PACK
  {"pack",    luaopen_pack},              // pack.pack/pack.unpack
#endif
  // {"mqttcore",luaopen_mqttcore},          // MQTT 协议封装
  // {"libcoap", luaopen_libcoap},           // 处理COAP消息

#ifdef LUAT_USE_LIBGNSS
  {"libgnss", luaopen_libgnss},           // 处理GNSS定位数据
#endif
#ifdef LUAT_USE_FS
  {"fs",      luaopen_fs},                // 文件系统库,在io库之外再提供一些方法
#endif
#ifdef LUAT_USE_SENSOR
  {"sensor",  luaopen_sensor},            // 传感器库,支持DS18B20
#endif
#ifdef LUAT_USE_SFUD
  {"sfud", luaopen_sfud},              // sfud
#endif
#ifdef LUAT_USE_SFD
  {"sfd", luaopen_sfd},              // sfud
#endif
#ifdef LUAT_USE_DISP
  {"disp",  luaopen_disp},              // OLED显示模块,支持SSD1306
#endif
#ifdef LUAT_USE_U8G2
  {"u8g2", luaopen_u8g2},              // u8g2
#endif

#ifdef LUAT_USE_EINK
  {"eink",  luaopen_eink},              // 电子墨水屏,试验阶段
#endif

#ifdef LUAT_USE_LVGL
// #ifndef LUAT_USE_LCD
// #define LUAT_USE_LCD
// #endif
  {"lvgl",   luaopen_lvgl},
#endif

#ifdef LUAT_USE_LCD
  {"lcd",    luaopen_lcd},
#endif
#ifdef LUAT_USE_STATEM
  {"statem",    luaopen_statem},
#endif
#ifdef LUAT_USE_GTFONT
  {"gtfont",    luaopen_gtfont},
#endif
#ifdef LUAT_USE_FSKV
  {"fskv", luaopen_fskv},
// 启用FSKV的时候,自动禁用FDB
#ifdef LUAT_USE_FDB
#undef LUAT_USE_FDB
#endif
#endif
#ifdef LUAT_USE_FDB
  {"fdb",       luaopen_fdb},
#endif
#ifdef LUAT_USE_VMX
  {"vmx",       luaopen_vmx},
#endif
#ifdef LUAT_USE_COREMARK
  {"coremark", luaopen_coremark},
#endif
#ifdef LUAT_USE_FONTS
  {"fonts", luaopen_fonts},
#endif
#ifdef LUAT_USE_ZLIB
  {"zlib", luaopen_zlib},
#endif
#ifdef LUAT_USE_MLX90640
  {"mlx90640", luaopen_mlx90640},
#endif
#ifdef LUAT_USE_IR
  {"ir", luaopen_ir},
#endif
#ifdef LUAT_USE_YMODEM
  {"ymodem", luaopen_ymodem},
#endif
#ifdef LUAT_USE_I2S
  {"i2s", luaopen_i2s},
#endif
#ifdef LUAT_USE_LORA
  {"lora", luaopen_lora},
#endif
#ifdef LUAT_USE_LORA2
  {"lora2", luaopen_lora2},
#endif
#ifdef LUAT_USE_MINIZ
  {"miniz", luaopen_miniz},
#endif
#ifdef LUAT_USE_PROTOBUF
  {"protobuf", luaopen_protobuf},
#endif
#ifdef LUAT_USE_IOTAUTH
  {"iotauth", luaopen_iotauth},
#endif
#ifdef LUAT_USE_WLAN
  {"wlan", luaopen_wlan},
#endif
#ifdef LUAT_USE_NETWORK
  {"http", luaopen_http},
  {"mqtt", luaopen_mqtt},
  {"socket", luaopen_socket_adapter},
  {"websocket", luaopen_websocket},
#ifdef LUAT_USE_FTP
  {"ftp", luaopen_ftp},
#endif
#ifdef LUAT_USE_W5500
  {"w5500", luaopen_w5500},
#endif
#endif
#ifdef LUAT_USE_HTTPSRV
  {"httpsrv", luaopen_httpsrv},
#endif
#ifdef LUAT_USE_NIMBLE
#if CONFIG_BT_ENABLED
  {"nimble", luaopen_nimble},
#endif
#endif
#ifdef LUAT_USE_RSA
  {"rsa", luaopen_rsa},
#endif
#ifdef LUAT_USE_FATFS
  {"fatfs", luaopen_fatfs},
#endif
#ifdef LUAT_USE_MAX30102
  {"max30102", luaopen_max30102},
#endif
#ifdef LUAT_USE_ICONV
  {"iconv", luaopen_iconv},
#endif
#ifdef LUAT_USE_BIT64
  {"bit64", luaopen_bit64},
#endif
#ifdef LUAT_USE_GMSSL
  {"gmssl", luaopen_gmssl},
#endif
#ifdef LUAT_USE_REPL
  {"repl", luaopen_repl},
#endif
#ifdef LUAT_USE_XXTEA
  {"xxtea", luaopen_xxtea},
#endif
#ifdef LUAT_USE_ULWIP
  {"ulwip", luaopen_ulwip},
#endif
  {NULL, NULL}
};

// 按不同的rtconfig加载不同的库函数
void luat_openlibs(lua_State *L) {
    // 初始化队列服务
    luat_msgbus_init();
    //print_list_mem("done>luat_msgbus_init");
    // 加载系统库
    const luaL_Reg *lib;
    /* "require" functions from 'loadedlibs' and set results to global table */
    for (lib = loadedlibs; lib->func; lib++) {
        luaL_requiref(L, lib->name, lib->func, 1);
        lua_pop(L, 1);  /* remove lib */
        //extern void print_list_mem(const char* name);
        //print_list_mem(lib->name);
    }
}

#include "esp_system.h"

void luat_os_reboot(int code){
    esp_restart();
}

const char* luat_os_bsp(void) {
#if defined(CONFIG_IDF_TARGET_ESP32)
    return "ESP32";
#elif defined(CONFIG_IDF_TARGET_ESP32C2)
    return "ESP32C2";
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
    return "ESP32C3";
#elif defined(CONFIG_IDF_TARGET_ESP32S2)
    return "ESP32S2";
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
    return "ESP32S3";
#else
    return "ESP32-UNKOWN";
#endif
}

/** 设备进入待机模式 */
void luat_os_standy(int timeout) {
    return; // nop
}

void luat_ota_reboot(int timeout_ms) {
  if (timeout_ms > 0)
    luat_timer_mdelay(timeout_ms);
  esp_restart();
}

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_rom_sys.h"


static uint8_t cri = 0;

#if defined(CONFIG_IDF_TARGET_ESP32)||defined(CONFIG_IDF_TARGET_ESP32S3)
portMUX_TYPE lock;
#endif

void luat_os_entry_cri(void) {
  if (cri == 0) {
#if defined(CONFIG_IDF_TARGET_ESP32)||defined(CONFIG_IDF_TARGET_ESP32S3)
    vPortExitCritical(&lock);
#else
    vPortExitCritical();
#endif
    cri = 1;
  }
}

void luat_os_exit_cri(void) {
  if (cri == 1) {
    cri = 0;
#if defined(CONFIG_IDF_TARGET_ESP32)||defined(CONFIG_IDF_TARGET_ESP32S3)
    vPortExitCritical(&lock);
#else
    vPortExitCritical();
#endif
  }
}

void luat_timer_us_delay(size_t us) {
  esp_rom_delay_us(us);
}
