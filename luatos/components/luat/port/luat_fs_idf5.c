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
#include "esp_flash.h"
#include "spi_flash_mmap.h"

#define LUAT_LOG_TAG "fs"
#include "luat_log.h"
#include "luat_ota.h"

#include <sys/types.h>
#include <dirent.h>

#ifdef LUAT_USE_LVGL
static void lvgl_fs_codec_init(void);
#endif

extern const struct luat_vfs_filesystem vfs_fs_spiffs;
extern const struct luat_vfs_filesystem vfs_fs_luadb;
extern const struct luat_vfs_filesystem vfs_fs_romfs;

static const void *map_ptr;
static spi_flash_mmap_handle_t map_handle;
const esp_partition_t * luadb_partition;

// 文件系统初始化函数
static const esp_vfs_spiffs_conf_t spiffs_conf = {
    .base_path = "/spiffs",
    .partition_label = NULL,
    .max_files = 10,
    .format_if_mount_failed = true};

int luat_fs_init(void) {
	esp_err_t ret = esp_vfs_spiffs_register(&spiffs_conf);
	if (ret) {
		LLOGW("spiffs register ret %d %s", ret, esp_err_to_name(ret));
	}
	// vfs进行必要的初始化
	luat_vfs_init(NULL);
	// 注册vfs for spiffs 实现
	luat_vfs_reg(&vfs_fs_spiffs);
	luat_fs_conf_t conf = {
		.busname = "",
		.type = "spiffs",
		.filesystem = "spiffs",
		.mount_point = "",
	};
	luat_fs_mount(&conf);
	// 先注册luadb
	luat_fs_conf_t conf2 = {0};
	luadb_partition = esp_partition_find_first(0x5A, 0x5A, "script");
	if (luadb_partition != NULL) {
		esp_partition_mmap(luadb_partition, 0, luadb_partition->size, SPI_FLASH_MMAP_DATA, &map_ptr, &map_handle);
    	conf2.busname = (char*)map_ptr;
        if (!memcmp(map_ptr, "-rom1fs-", 8)) {
            LLOGI("script zone as romfs");
            conf2.type = "romfs";
		    conf2.filesystem = "romfs";
		    conf2.mount_point = "/luadb/";
	        luat_vfs_reg(&vfs_fs_romfs);
        }
        else {
            LLOGI("script zone as luadb");
		    conf2.type = "luadb",
		    conf2.filesystem = "luadb",
		    conf2.mount_point = "/luadb/",
	        luat_vfs_reg(&vfs_fs_luadb);
        }
        luat_fs_mount(&conf2);
	}
	else {
		LLOGE("script partition NOT Found !!!");
	}
#ifdef LUAT_USE_LVGL
    lvgl_fs_codec_init();
#endif
    return 0;
}

FILE* luat_vfs_spiffs_fopen(void* userdata, const char *filename, const char *mode) {
    char path[256] = {0};
	if (filename == NULL || strlen(filename) > 240) return NULL;
	if (filename[0] == '/')
		sprintf(path, "/spiffs%s", filename);
	else
		sprintf(path, "/spiffs/%s", filename);
    return fopen(path, mode);
}

int luat_vfs_spiffs_getc(void* userdata, FILE* stream) {
    uint8_t buff = 0;
    int ret = luat_fs_fread(&buff, 1, 1, stream);
    if (ret == 1) {
        return buff;
    }
    return -1;
}

int luat_vfs_spiffs_fseek(void* userdata, FILE* stream, long int offset, int origin) {
    return fseek(stream, offset, origin);
}

int luat_vfs_spiffs_ftell(void* userdata, FILE* stream) {
    return ftell(stream);
}

int luat_vfs_spiffs_fclose(void* userdata, FILE* stream) {
    return fclose(stream);
}
int luat_vfs_spiffs_feof(void* userdata, FILE* stream) {
    return feof(stream);
}
int luat_vfs_spiffs_ferror(void* userdata, FILE *stream) {
    return ferror(stream);
}
size_t luat_vfs_spiffs_fread(void* userdata, void *ptr, size_t size, size_t nmemb, FILE *stream) {
    int ret = fread(ptr, size, nmemb, stream);
	if (ret > 0)
		return size * nmemb;
	return 0;
}
size_t luat_vfs_spiffs_fwrite(void* userdata, const void *ptr, size_t size, size_t nmemb, FILE *stream) {
    int ret = fwrite(ptr, size, nmemb, stream);
	if (ret > 0)
		return size * nmemb;
	return 0;
}
int luat_vfs_spiffs_remove(void* userdata, const char *filename) {
    char path[256] = {0};
	if (filename == NULL || strlen(filename) > 240) return 0;
	if (filename[0] == '/')
		sprintf(path, "/spiffs%s", filename);
	else
		sprintf(path, "/spiffs/%s", filename);
    return remove(path);
}
int luat_vfs_spiffs_rename(void* userdata, const char *old_filename, const char *new_filename) {
    char path[256] = {0};
    char path2[256] = {0};
	if (old_filename == NULL || strlen(old_filename) > 240) return -1;
	if (new_filename == NULL || strlen(new_filename) > 240) return -1;
	if (old_filename[0] == '/')
		sprintf(path, "/spiffs%s", old_filename);
	else
		sprintf(path, "/spiffs/%s", old_filename);
	if (new_filename[0] == '/')
		sprintf(path2, "/spiffs%s", new_filename);
	else
		sprintf(path2, "/spiffs/%s", new_filename);
    return rename(path, path2);
}
int luat_vfs_spiffs_fexist(void* userdata, const char *filename) {
    FILE* fd = luat_vfs_spiffs_fopen(userdata, filename, "rb");
    if (fd) {
        luat_vfs_spiffs_fclose(userdata, fd);
        return 1;
    }
    return 0;
}

size_t luat_vfs_spiffs_fsize(void* userdata, const char *filename) {
    FILE *fd;
    size_t size = 0;
    fd = luat_vfs_spiffs_fopen(userdata, filename, "rb");
    if (fd) {
        luat_vfs_spiffs_fseek(userdata, fd, 0, SEEK_END);
        size = luat_vfs_spiffs_ftell(userdata, fd); 
        luat_vfs_spiffs_fclose(userdata, fd);
    }
    return size;
}

int luat_vfs_spiffs_mkfs(void* userdata, luat_fs_conf_t *conf) {
    return -1;
}
int luat_vfs_spiffs_mount(void** userdata, luat_fs_conf_t *conf) {
    return 0;
}
int luat_vfs_spiffs_umount(void* userdata, luat_fs_conf_t *conf) {
    return 0;
}

int luat_vfs_spiffs_mkdir(void* userdata, char const* dir) {
    char path[256] = {0};
	if (dir == NULL || strlen(dir) > 240) return -1;
	if (dir[0] == '/')
		sprintf(path, "/spiffs%s", dir);
	else
		sprintf(path, "/spiffs/%s", dir);
    return mkdir(path, 0);
}
int luat_vfs_spiffs_rmdir(void* userdata, char const* dir) {
    char path[256] = {0};
	if (dir == NULL || strlen(dir) > 240) return -1;
	if (dir[0] == '/')
		sprintf(path, "/spiffs%s", dir);
	else
		sprintf(path, "/spiffs/%s", dir);
	return remove(path);
}
int luat_vfs_spiffs_info(void* userdata, const char* path, luat_fs_info_t *conf) {
    memcpy(conf->filesystem, "spiffs", strlen("spiffs")+1);
    conf->type = 0;
    conf->total_block = 0;
    conf->block_used = 0;
    conf->block_size = 512;
    return 0;
}

int luat_vfs_spiffs_lsdir(void* fsdata, char const* dir, luat_fs_dirent_t* ents, size_t offset, size_t len) {
    DIR *dp;
    struct dirent *ep;
    int index = 0;


    char path[256] = {0};
	if (dir == NULL || strlen(dir) > 240) return 0;
	if (dir[0] == '/')
		sprintf(path, "/spiffs%s", dir);
	else
		sprintf(path, "/spiffs/%s", dir);

    dp = opendir (path);
    if (dp != NULL)
    {
        while ((ep = readdir (dp)) != NULL) {
            //LLOGW("offset %d len %d", offset, len);
            if (offset > 0) {
                offset --;
                continue;
            }
            if (len > 0) {
                memcpy(ents[index].d_name, ep->d_name, strlen(ep->d_name) + 1);
                ents[index].d_type = 0;
                index++;
                len --;
            }
            else {
                break;
            }
        }

        (void) closedir (dp);
        return index;
    }
    else {
        LLOGW("opendir file %s", dir);
    }
    return 0;
}

#define T(name) .name = luat_vfs_spiffs_##name
const struct luat_vfs_filesystem vfs_fs_spiffs = {
    .name = "spiffs",
    .opts = {
        .mkfs = NULL,
        T(mount),
        T(umount),
        T(mkdir),
        T(rmdir),
        T(lsdir),
        T(remove),
        T(rename),
        T(fsize),
        T(fexist),
        T(info)
    },
    .fopts = {
        T(fopen),
        T(getc),
        T(fseek),
        T(ftell),
        T(fclose),
        T(feof),
        T(ferror),
        T(fread),
        T(fwrite)
    }
};



#ifdef LUAT_USE_LVGL
#include "lvgl.h"
#include "luat_lvgl.h"
#include "lv_sjpg.h"
static void lvgl_fs_codec_init(void) {
	luat_lv_fs_init();
	// lv_bmp_init();
	// lv_png_init();
	lv_split_jpeg_init();
}

#endif


