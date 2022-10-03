#include "luat_base.h"
#include "luat_sfd.h"

#include "esp_err.h"
#include "esp_system.h"
#include "esp_partition.h"

#define LUAT_LOG_TAG "sfd"
#include "luat_log.h"

// 存放fdb分区的引用
static const esp_partition_t * fdb_partition;

// 初始化, 把分区读出来就行
int sfd_onchip_init (void* userdata) {
    fdb_partition = esp_partition_find_first(0x5A, 0x5B, "fdb");
    if (fdb_partition == NULL) {
        LLOGE("fdb partition NOT Found");
        return -1;
    }
    return 0;
}

// 读取状态
int sfd_onchip_status (void* userdata) {
    return fdb_partition == NULL ? -1 : 0;
}

// 下面就是flash的常规操作了, read/write/erase/ioctl
int sfd_onchip_read (void* userdata, char* buff, size_t offset, size_t len) {
    int ret = esp_partition_read(fdb_partition,  offset, buff, len);
    return ret >= 0 ? len : -1;
}

int sfd_onchip_write (void* userdata, const char* buff, size_t offset, size_t len) {
    int ret = esp_partition_write(fdb_partition, offset, buff, len);
    return ret >= 0 ? len : -1;
}

int sfd_onchip_erase (void* userdata, size_t offset, size_t len) {
    int ret = esp_partition_erase_range(fdb_partition, offset, len);
    return ret >= 0 ? len : -1;
}

int sfd_onchip_ioctl (void* userdata, size_t cmd, void* buff) {
    // 不支持, 也用不到
    return 0;
}
