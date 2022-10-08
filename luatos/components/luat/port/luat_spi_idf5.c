
#include "luat_base.h"
#include "luat_gpio.h"
#include "luat_spi.h"
#include "luat_malloc.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"

#define LUAT_LOG_TAG "spi"
#include "luat_log.h"

static spi_device_handle_t spi_handle = {0};

int luat_spi_setup(luat_spi_t *spi){
    esp_err_t ret = -1;
    if (spi->id == 2){
        spi_bus_config_t buscfg = {
            .miso_io_num = 10,
            .mosi_io_num = 3,
            .sclk_io_num = 2,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = SOC_SPI_MAXIMUM_BUFFER_SIZE
        };
        ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
        if (ret != 0){
            return ret;
        }
        spi_device_interface_config_t dev_config;
        memset(&dev_config, 0, sizeof(dev_config));
        if (spi->CPHA == 0){
            if (spi->CPOL == 0)
                dev_config.mode = 0;
            if (spi->CPOL == 1)
                dev_config.mode = 1;
        }else{
            if (spi->CPOL == 0)
                dev_config.mode = 2;
            if (spi->CPOL == 1)
                dev_config.mode = 3;
        }
        dev_config.clock_speed_hz = spi->bandrate;
        if (spi->cs == Luat_GPIO_MAX_ID)
            dev_config.spics_io_num = -1;
        else
            dev_config.spics_io_num = spi->cs;
        dev_config.queue_size = 7;
        ret = spi_bus_add_device(SPI2_HOST, &dev_config, &spi_handle);
        return ret;
    }
    else
        return -1;
}

int luat_spi_close(int spi_id){
    esp_err_t ret = -1;
    if (spi_id == 2){
        ret = spi_bus_remove_device(spi_handle);
        if (ret != 0){
            return ret;
        }
        ret = spi_bus_free(SPI2_HOST);
        return ret;
    }
    else
        return -1;
}

int luat_spi_transfer(int spi_id, const char *send_buf, size_t send_length, char *recv_buf, size_t recv_length){
    esp_err_t ret = -1;
    if (spi_id == 2){
        spi_transaction_t send;
        memset(&send, 0, sizeof(send));
        while (send_length > 0) {
            memset(&send, 0, sizeof(send));
            if (send_length > SOC_SPI_MAXIMUM_BUFFER_SIZE ) { 
                send.length = SOC_SPI_MAXIMUM_BUFFER_SIZE  * 8;
                send.tx_buffer = send_buf;
                ret = spi_device_polling_transmit(spi_handle, &send);
                send_buf += SOC_SPI_MAXIMUM_BUFFER_SIZE ;
                send_length -= SOC_SPI_MAXIMUM_BUFFER_SIZE ;
            }
            else {
                send.length = send_length * 8;
                send.tx_buffer = send_buf;
                ret = spi_device_polling_transmit(spi_handle, &send);
                break;
            }
        }
        if (ret != 0){
            return -2;
        }
        spi_transaction_t recv;
        memset(&recv, 0, sizeof(recv));
        recv.length = recv_length * 8;
        recv.rxlength = recv_length * 8;
        recv.rx_buffer = recv_buf;
        ret = spi_device_polling_transmit(spi_handle, &recv);
        return ret == 0 ? recv_length : -1;
    }
    else
        return -1;
}

int luat_spi_recv(int spi_id, char *recv_buf, size_t length){
    esp_err_t ret = -1;
    if (spi_id == 2){
        spi_transaction_t t;
        memset(&t, 0, sizeof(t));
        t.length = length * 8;
        t.rxlength = length * 8;
        t.rx_buffer = recv_buf;
        ret = spi_device_polling_transmit(spi_handle, &t);
        return ret == 0 ? length : -1;
    }
    else
        return -1;
}

int luat_spi_send(int spi_id, const char *send_buf, size_t length){
    spi_transaction_t t;
    esp_err_t ret = -1;
    if (spi_id == 2){
        while (length > 0) {
            memset(&t, 0, sizeof(t));
            if (length > SOC_SPI_MAXIMUM_BUFFER_SIZE ) { 
                t.length = SOC_SPI_MAXIMUM_BUFFER_SIZE  * 8;
                t.tx_buffer = send_buf;
                ret = spi_device_polling_transmit(spi_handle, &t);
                send_buf += SOC_SPI_MAXIMUM_BUFFER_SIZE ;
                length -= SOC_SPI_MAXIMUM_BUFFER_SIZE ;
            }
            else {
                t.length = length * 8;
                t.tx_buffer = send_buf;
                ret = spi_device_polling_transmit(spi_handle, &t);
                break;
            }
        }
        return ret == 0 ? length : -1;
    }
    else
        return -1;
}

#define LUAT_SPI_CS_SELECT 0
#define LUAT_SPI_CS_CLEAR 1

static uint8_t spi_bus = 0;

int luat_spi_device_setup(luat_spi_device_t *spi_dev){
    esp_err_t ret = -1;
    int bus_id = spi_dev->bus_id;
    spi_device_handle_t *spi_device = luat_heap_malloc(sizeof(spi_device_handle_t));
    if (spi_device == NULL)
        return ret;
    spi_dev->user_data = (void *)spi_device;
    if (bus_id == 2 && spi_bus == 0){
        spi_bus_config_t buscfg = {
            .miso_io_num = 10,
            .mosi_io_num = 3,
            .sclk_io_num = 2,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = SOC_SPI_MAXIMUM_BUFFER_SIZE
        };
        ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
        if (ret != 0){
            return ret;
        }
        spi_bus = 1;
    }
    spi_device_interface_config_t dev_config;
    memset(&dev_config, 0, sizeof(dev_config));
    if (spi_dev->spi_config.CPHA == 0){
        if (spi_dev->spi_config.CPOL == 0)
            dev_config.mode = 0;
        if (spi_dev->spi_config.CPOL == 1)
            dev_config.mode = 1;
    }else{
        if (spi_dev->spi_config.CPOL == 0)
            dev_config.mode = 2;
        if (spi_dev->spi_config.CPOL == 1)
            dev_config.mode = 3;
    }
    dev_config.clock_speed_hz = spi_dev->spi_config.bandrate;
    dev_config.spics_io_num = -1; 
    dev_config.queue_size = 7;
    if (spi_dev->spi_config.mode == 0)
        dev_config.flags = SPI_DEVICE_HALFDUPLEX;
    if (bus_id == 2)
        ret = spi_bus_add_device(SPI2_HOST, &dev_config, spi_device);
    if (ret != 0)
        luat_heap_free(spi_device);
    luat_gpio_mode(spi_dev->spi_config.cs, Luat_GPIO_OUTPUT, Luat_GPIO_DEFAULT, Luat_GPIO_HIGH);
    return ret;
}

int luat_spi_device_close(luat_spi_device_t *spi_dev){
    esp_err_t ret = -1;
    int bus_id = spi_dev->bus_id;
    if (bus_id == 2){
        ret = spi_bus_remove_device(*(spi_device_handle_t *)(spi_dev->user_data));
    }
    luat_heap_free((spi_device_handle_t *)(spi_dev->user_data));
    return ret;
}

int luat_spi_device_transfer(luat_spi_device_t *spi_dev, const char *send_buf, size_t send_length, char *recv_buf, size_t recv_length){
    luat_gpio_set(spi_dev->spi_config.cs, LUAT_SPI_CS_SELECT);
    esp_err_t ret = -1;
    int bus_id = spi_dev->bus_id;
    if (bus_id == 2){
        spi_transaction_t send;
        memset(&send, 0, sizeof(send));
        while (send_length > 0) {
            memset(&send, 0, sizeof(send));
            if (send_length > SOC_SPI_MAXIMUM_BUFFER_SIZE ) {
                send.length = SOC_SPI_MAXIMUM_BUFFER_SIZE  * 8;
                send.tx_buffer = send_buf;
                ret = spi_device_polling_transmit(*(spi_device_handle_t *)(spi_dev->user_data), &send);
                send_buf += SOC_SPI_MAXIMUM_BUFFER_SIZE ;
                send_length -= SOC_SPI_MAXIMUM_BUFFER_SIZE ;
            }else {
                send.length = send_length * 8;
                send.tx_buffer = send_buf;
                ret = spi_device_polling_transmit(*(spi_device_handle_t *)(spi_dev->user_data), &send);
                break;
            }
        }
        if (ret != 0){
            return -2;
        }
        spi_transaction_t recv;
        memset(&recv, 0, sizeof(recv));
        recv.length = recv_length * 8;
        recv.rxlength = recv_length * 8;
        recv.rx_buffer = recv_buf;
        ret = spi_device_polling_transmit(*(spi_device_handle_t *)(spi_dev->user_data), &recv);
    }
    luat_gpio_set(spi_dev->spi_config.cs, LUAT_SPI_CS_CLEAR);
    return ret == 0 ? recv_length : -1;
}

int luat_spi_device_recv(luat_spi_device_t *spi_dev, char *recv_buf, size_t length){
    luat_gpio_set(spi_dev->spi_config.cs, LUAT_SPI_CS_SELECT);
    esp_err_t ret = -1;
    int bus_id = spi_dev->bus_id;
    if (bus_id == 2){
        spi_transaction_t t;
        memset(&t, 0, sizeof(t));
        t.length = length * 8;
        t.rxlength = length * 8;
        t.rx_buffer = recv_buf;
        ret = spi_device_polling_transmit(*(spi_device_handle_t *)(spi_dev->user_data), &t);
    }
    luat_gpio_set(spi_dev->spi_config.cs, LUAT_SPI_CS_CLEAR);
    return ret == 0 ? length : -1;
}

int luat_spi_device_send(luat_spi_device_t *spi_dev, const char *send_buf, size_t length){
    luat_gpio_set(spi_dev->spi_config.cs, LUAT_SPI_CS_SELECT);
    esp_err_t ret = -1;
    int bus_id = spi_dev->bus_id;
    if (bus_id == 2){
        spi_transaction_t t;
        memset(&t, 0, sizeof(t));
        while (length > 0) {
            memset(&t, 0, sizeof(t));
            if (length > SOC_SPI_MAXIMUM_BUFFER_SIZE ) { 
                t.length = SOC_SPI_MAXIMUM_BUFFER_SIZE  * 8;
                t.tx_buffer = send_buf;
                ret = spi_device_polling_transmit(*(spi_device_handle_t *)(spi_dev->user_data), &t);
                send_buf += SOC_SPI_MAXIMUM_BUFFER_SIZE ;
                length -= SOC_SPI_MAXIMUM_BUFFER_SIZE ;
            }else {
                t.length = length * 8;
                t.tx_buffer = send_buf;
                ret = spi_device_polling_transmit(*(spi_device_handle_t *)(spi_dev->user_data), &t);
                break;
            }
        }
    }
    luat_gpio_set(spi_dev->spi_config.cs, LUAT_SPI_CS_CLEAR);
    return ret == 0 ? length : -1;
}

int luat_spi_device_config(luat_spi_device_t* spi_dev){
    return 0;
}
