#include "luat_base.h"
#include "luat_fs.h"
#include "luat_msgbus.h"
#include "luat_ota.h"

#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_spiffs.h"
#include "esp_system.h"
#include "esp_partition.h"

#define LUAT_LOG_TAG "fs"
#include "luat_log.h"

#ifdef LUAT_USE_LVGL
#include "lvgl.h"
#include "luat_lvgl.h"
#include "lv_sjpg.h"
#endif

extern const struct luat_vfs_filesystem vfs_fs_posix;
extern const struct luat_vfs_filesystem vfs_fs_luadb;

static const void *map_ptr;
static spi_flash_mmap_handle_t map_handle;
static const esp_partition_t * partition;

// 文件系统初始化函数
static const esp_vfs_spiffs_conf_t spiffs_conf = {
    .base_path = "/",
    .partition_label = NULL,
    .max_files = 10,
    .format_if_mount_failed = true};

int luat_fs_init(void) {
	esp_err_t ret = esp_vfs_spiffs_register(&spiffs_conf);
	if (ret) {
		LLOGW("spiffs register ret %d", ret);
	}
	// vfs进行必要的初始化
	luat_vfs_init(NULL);
	// 注册vfs for posix 实现
	luat_vfs_reg(&vfs_fs_posix);
	luat_fs_conf_t conf = {
		.busname = "",
		.type = "posix",
		.filesystem = "posix",
		.mount_point = "",
	};
	luat_fs_mount(&conf);
	// 先注册luadb
	luat_vfs_reg(&vfs_fs_luadb);
	luat_fs_conf_t conf2 = {
		.busname = (char*)NULL,
		.type = "luadb",
		.filesystem = "luadb",
		.mount_point = "/luadb/",
	};
	partition = esp_partition_find_first(0x5A, 0x5A, "script");
	if (partition != NULL) {
		esp_partition_mmap(partition, 0, partition->size, SPI_FLASH_MMAP_DATA, &map_ptr, &map_handle);
    	conf2.busname = (char*)map_ptr;
    	luat_fs_mount(&conf2);
	}
	else {
		LLOGE("script partition NOT Found !!!");
	}
#ifdef LUAT_USE_LVGL
	luat_lv_fs_init();
	// lv_bmp_init();
	// lv_png_init();
	lv_split_jpeg_init();
#endif
    return 0;
}
