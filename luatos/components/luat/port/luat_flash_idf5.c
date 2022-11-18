#include "luat_base.h"
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

#define LUAT_LOG_TAG "flash"
#include "luat_log.h"


extern const esp_partition_t * luadb_partition;

int luat_flash_read(char* buff, size_t addr, size_t len) {
    if (luadb_partition == NULL) {
        LLOGE("luadb_partition == NULL");
        return 0;
    }
    if (luadb_partition->address > addr || addr >= luadb_partition->address + luadb_partition->size) {
        LLOGW("read out of range");
        return 0;
    }
    if (addr + len > luadb_partition->address + luadb_partition->size) {
        len = luadb_partition->address + luadb_partition->size - addr;
    }
    int ret = esp_partition_read(luadb_partition, addr - luadb_partition->address, buff, len);
    if (ret == ESP_OK) {
        return len;
    }
    else {
        LLOGW("read ret %d", ret);
    }
    return 0;
}

int luat_flash_write(char* buff, size_t addr, size_t len) {
    if (luadb_partition == NULL) {
        LLOGE("luadb_partition == NULL");
        return 0;
    }
    //LLOGD("addr %08X size %04X luadb %08X size %08X", addr, len, luadb_partition->address, luadb_partition->size);
    if (luadb_partition->address > addr || addr >= (luadb_partition->address + luadb_partition->size)) {
        LLOGW("write out of range");
        return 0;
    }
    if ((addr + len) > (luadb_partition->address + luadb_partition->size)) {
        len = luadb_partition->address + luadb_partition->size - addr;
    }
    int ret = esp_partition_write(luadb_partition, addr - luadb_partition->address, buff, len);
    if (ret == ESP_OK) {
        return len;
    }
    else {
        LLOGW("write ret %d", ret);
    }
    return 0;
}

int luat_flash_erase(size_t addr, size_t len) {
    if (luadb_partition == NULL) {
        LLOGE("luadb_partition == NULL");
        return 0;
    }
    if (luadb_partition->address > addr || (addr >= luadb_partition->address + luadb_partition->size)) {
        LLOGW("erase out of range");
        return 0;
    }
    if (addr + len > luadb_partition->address + luadb_partition->size) {
        len = luadb_partition->address + luadb_partition->size - addr;
    }
    int ret = esp_partition_erase_range(luadb_partition, addr - luadb_partition->address, len);
    if (ret == ESP_OK) {
        return len;
    }
    else {
        LLOGW("erase ret %d", ret);
    }
    return 0;
}
