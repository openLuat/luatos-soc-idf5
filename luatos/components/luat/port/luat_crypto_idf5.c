
#include "luat_base.h"
#include "luat_crypto.h"

// 使用ESP自带的随机函数
#include "esp_random.h"

int luat_crypto_trng(char *buff, size_t len)
{
    // 直接调用就行了
    esp_fill_random(buff, len);
    return 0;
}
