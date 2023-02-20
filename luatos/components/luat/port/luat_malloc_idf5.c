
// 这个文件包含 系统heap和lua heap的默认实现


#include <stdlib.h>
#include <string.h>//add for memset
#include "bget.h"
#include "luat_malloc.h"
#include "esp_system.h"
#include "esp_attr.h"

#define LUAT_LOG_TAG "vmheap"
#include "luat_log.h"


//-----------------------------------------------------------------------------

#if defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32S3)
#define LUAT_HEAP_SIZE (96*1024)
#elif defined(CONFIG_IDF_TARGET_ESP32C2)
#define LUAT_HEAP_SIZE (68*1024)
#else
#define LUAT_HEAP_SIZE (64*1024)
#endif
static uint8_t vmheap[LUAT_HEAP_SIZE];
#if LUAT_USE_MEMORY_OPTIMIZATION_CODE_MMAP
static uint32_t heap_addr_start = (uint32_t) vmheap;
static uint32_t heap_addr_end = (uint32_t) vmheap + LUAT_HEAP_SIZE;
#endif

//------------------------------------------------
//  管理系统内存

void* luat_heap_malloc(size_t len) {
    return malloc(len);
}

void luat_heap_free(void* ptr) {
    free(ptr);
}

void* luat_heap_realloc(void* ptr, size_t len) {
    return realloc(ptr, len);
}

void* luat_heap_calloc(size_t count, size_t _size) {
    return calloc(count, _size);
}
//------------------------------------------------

//------------------------------------------------
// ---------- 管理 LuaVM所使用的内存----------------

#if 1
void* IRAM_ATTR luat_heap_alloc(void *ud, void *ptr, size_t osize, size_t nsize) {
    if (ptr == NULL && nsize == 0)
        return NULL;
#if LUAT_USE_MEMORY_OPTIMIZATION_CODE_MMAP
    if (ptr != NULL && nsize == 0) {
        uint32_t addr = (uint32_t) ptr;
        if (addr < heap_addr_start || addr > heap_addr_end) {
            //LLOGD("skip ROM free %p", ptr);
            return NULL;
        }
    }
#endif

    if (nsize)
    {
    	void* ptmp = bgetr(ptr, nsize);
    	if(ptmp == NULL && osize >= nsize)
        {
            return ptr;
        }
        return ptmp;
    }
    brel(ptr);
    return NULL;
}

void luat_meminfo_luavm(size_t *total, size_t *used, size_t *max_used) {
    long curalloc, totfree, maxfree;
    unsigned long nget, nrel;
    bstats(&curalloc, &totfree, &maxfree, &nget, &nrel);
    *used = curalloc;
    *max_used = bstatsmaxget();
    *total = curalloc + totfree;
}

#else
#include "heap_tlsf.h"
static tlsf_t vm_tlfs;

void* luat_heap_alloc(void *ud, void *ptr, size_t osize, size_t nsize) {
    if (ptr == NULL && nsize == 0)
        return NULL;
#if LUAT_USE_MEMORY_OPTIMIZATION_CODE_MMAP
    if (ptr != NULL && nsize == 0) {
        uint32_t addr = (uint32_t) ptr;
        if (addr < heap_addr_start || addr > heap_addr_end) {
            LLOGD("skip ROM free %p", ptr);
            return NULL;
        }
    }
#endif
    return tlsf_realloc(ptr, nsize);
}
void luat_meminfo_luavm(size_t *total, size_t *used, size_t *max_used) {
    *total = 0;
    *used = 0;
    *max_used = 0;
}
#endif

#include "esp_system.h"
#include "esp_heap_caps.h"
#include "esp_psram.h"
void luat_meminfo_sys(size_t *total, size_t *used, size_t *max_used)
{
    *total = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);
    *used = *total - heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
    *max_used = *total - heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT);
}

void luat_heap_init(void)
{
#ifdef LUAT_USE_PSRAM
    size_t t = esp_psram_get_size();
    LLOGD("Found %d kbyte PSRAM", t / 1024);
    #define LUAT_HEAP_PSRAM_SIZE (4 * 1024 * 1024)
    if (t > 0)
    {
        char* ptr = heap_caps_malloc(LUAT_HEAP_PSRAM_SIZE, MALLOC_CAP_SPIRAM);
        #if LUAT_USE_MEMORY_OPTIMIZATION_CODE_MMAP
        heap_addr_start = (uint32_t)ptr;
        heap_addr_end = (uint32_t)ptr + LUAT_HEAP_PSRAM_SIZE;
        #endif
        bpool(ptr, LUAT_HEAP_PSRAM_SIZE);
    }
    else
    {
        bpool(vmheap, LUAT_HEAP_SIZE);
    }
#else
    bpool(vmheap, LUAT_HEAP_SIZE);
#endif
    // LLOGD("vm heap range %08X %08X", heap_addr_start, heap_addr_end);
}
