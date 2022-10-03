/* Esptouch example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "bget.h"

#define LUAT_HEAP_SIZE (64*1024)
static uint8_t vmheap[LUAT_HEAP_SIZE];

extern int luat_main (void);
extern void bootloader_random_enable(void);

void app_main(void)
{
    bootloader_random_enable();
    ESP_ERROR_CHECK( nvs_flash_init() );
    bpool(vmheap, LUAT_HEAP_SIZE);
    luat_main();
}
