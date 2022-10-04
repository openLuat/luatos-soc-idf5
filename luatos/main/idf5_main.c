/* Esptouch example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include <stdlib.h>
#include "esp_event.h"
#include "esp_system.h"
#include "nvs_flash.h"


extern int luat_main (void);
extern void bootloader_random_enable(void);
extern void luat_heap_init(void);

void app_main(void)
{
    bootloader_random_enable();
    nvs_flash_init();
    luat_heap_init();
    esp_event_loop_create_default();
    luat_main();
}
