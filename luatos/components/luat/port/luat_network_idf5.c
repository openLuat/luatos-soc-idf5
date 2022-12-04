#include "luat_base.h"

#ifdef LUAT_USE_NETWORK
#include "luat_network_posix.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static void client_entry(void* args) {
    posix_socket_t *ps = (posix_socket_t*)args;
    posix_network_client_thread_entry(ps);
    vTaskDelete(NULL);
}

int network_posix_client_thread_start(posix_socket_t* ps){
    int err = xTaskCreate(client_entry, "socket", 4096, ps, 30, NULL);
    if (err == pdPASS)
        return 0;
    return -1;
}

#endif
