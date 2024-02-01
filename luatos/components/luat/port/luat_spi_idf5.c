
#include "luat_base.h"
#include "luat_gpio.h"
#include "luat_spi.h"
#include "luat_malloc.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"

#include "idf5_io_def.h"

#define LUAT_LOG_TAG "spi"
#include "luat_log.h"

#if defined(CONFIG_IDF_TARGET_ESP32)||defined(CONFIG_IDF_TARGET_ESP32S2)||defined(CONFIG_IDF_TARGET_ESP32S3)
#define SOC_SPI_NUM 2
#else
#define SOC_SPI_NUM 1
#endif

static spi_device_interface_config_t spi_config[SOC_SPI_NUM] = {0};
static spi_device_handle_t spi_handle[SOC_SPI_NUM] = {0};

int luat_spi_setup(luat_spi_t *spi){
    esp_err_t ret = -1;
    spi_bus_config_t buscfg = {
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = SOC_SPI_MAXIMUM_BUFFER_SIZE
    };
    if (spi->id == 2){
        buscfg.miso_io_num = SPI2_MISO_IO_NUM;
        buscfg.mosi_io_num = SPI2_MOSI_IO_NUM;
        buscfg.sclk_io_num = SPI2_SCLK_IO_NUM;
    }
#if defined(CONFIG_IDF_TARGET_ESP32)||defined(CONFIG_IDF_TARGET_ESP32S2)||defined(CONFIG_IDF_TARGET_ESP32S3)
    else if(spi->id == 3){
        buscfg.miso_io_num = SPI3_MISO_IO_NUM;
        buscfg.mosi_io_num = SPI3_MOSI_IO_NUM;
        buscfg.sclk_io_num = SPI3_SCLK_IO_NUM;
    }
#endif
    else{
        return -1;
    }
    ret = spi_bus_initialize(spi->id-1, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != 0){
        return ret;
    }
    spi_device_interface_config_t* dev_config = &spi_config[spi->id-2];
    memset(dev_config, 0, sizeof(spi_device_interface_config_t));
    if (spi->CPHA == 0){
        if (spi->CPOL == 0)
            dev_config->mode = 0;
        if (spi->CPOL == 1)
            dev_config->mode = 1;
    }else{
        if (spi->CPOL == 0)
            dev_config->mode = 2;
        if (spi->CPOL == 1)
            dev_config->mode = 3;
    }
    if (spi->mode == 0){
        dev_config->flags |= SPI_DEVICE_HALFDUPLEX;
    }
    dev_config->clock_speed_hz = spi->bandrate;
    if (spi->cs == Luat_GPIO_MAX_ID)
        dev_config->spics_io_num = -1;
    else
        dev_config->spics_io_num = spi->cs;
    dev_config->queue_size = 7;
    ret = spi_bus_add_device(spi->id-1, dev_config, &spi_handle[spi->id-2]);
    return ret;
}

int luat_spi_close(int spi_id){
    esp_err_t ret = -1;
#if defined(CONFIG_IDF_TARGET_ESP32)||defined(CONFIG_IDF_TARGET_ESP32S2)||defined(CONFIG_IDF_TARGET_ESP32S3)
    if (spi_id > 3 || spi_id < 2){
        return -1;
    }
#else
    if (spi_id != 2){
        return -1;
    }
#endif
    ret = spi_bus_remove_device(spi_handle[spi_id-2]);
    if (ret != 0){
        return ret;
    }
    ret = spi_bus_free(spi_id-1);
    return ret;
}

int luat_spi_transfer(int spi_id, const char *send_buf, size_t send_length, char *recv_buf, size_t recv_length){
    spi_transaction_t send = {0};
    spi_transaction_t recv = {0};
    esp_err_t ret = -1;
    size_t transmit_length = send_length;
#if defined(CONFIG_IDF_TARGET_ESP32)||defined(CONFIG_IDF_TARGET_ESP32S2)||defined(CONFIG_IDF_TARGET_ESP32S3)
    if (spi_id > 3 || spi_id < 2){
        return -1;
    }
#else
    if (spi_id != 2){
        return -1;
    }
#endif
    if ((spi_config[spi_id-2].flags & SPI_DEVICE_HALFDUPLEX) != 0){
        while (send_length > 0) {
            memset(&send, 0, sizeof(send));
            if (send_length > SOC_SPI_MAXIMUM_BUFFER_SIZE ) { 
                send.length = SOC_SPI_MAXIMUM_BUFFER_SIZE  * 8;
                send.tx_buffer = send_buf;
                ret = spi_device_polling_transmit(spi_handle[spi_id-2], &send);
                send_buf += SOC_SPI_MAXIMUM_BUFFER_SIZE ;
                send_length -= SOC_SPI_MAXIMUM_BUFFER_SIZE ;
            }
            else {
                send.length = send_length * 8;
                send.tx_buffer = send_buf;
                ret = spi_device_polling_transmit(spi_handle[spi_id-2], &send);
                break;
            }
        }
        if (ret != 0){
            return -2;
        }
        recv.length = recv_length * 8;
        recv.rxlength = recv_length * 8;
        recv.rx_buffer = recv_buf;
        ret = spi_device_polling_transmit(spi_handle[spi_id-2], &recv);
    }else{
        while (send_length > 0) {
            memset(&send, 0, sizeof(send));
            if (send_length > SOC_SPI_MAXIMUM_BUFFER_SIZE ) { 
                send.length = SOC_SPI_MAXIMUM_BUFFER_SIZE  * 8;
                send.tx_buffer = send_buf;
                send.rxlength = SOC_SPI_MAXIMUM_BUFFER_SIZE * 8;
                send.rx_buffer = recv_buf;
                ret = spi_device_polling_transmit(spi_handle[spi_id-2], &send);
                send_buf += SOC_SPI_MAXIMUM_BUFFER_SIZE ;
                send_length -= SOC_SPI_MAXIMUM_BUFFER_SIZE ;
                recv_buf += SOC_SPI_MAXIMUM_BUFFER_SIZE ;
                recv_length -= SOC_SPI_MAXIMUM_BUFFER_SIZE ;
            }
            else {
                send.length = send_length * 8;
                send.tx_buffer = send_buf;
                send.rxlength = send_length * 8;
                send.rx_buffer = recv_buf;
                ret = spi_device_polling_transmit(spi_handle[spi_id-2], &send);
                break;
            }
        }
        if (ret != 0){
            return -2;
        }
    }
    return ret == 0 ? transmit_length : -1;
}

int luat_spi_recv(int spi_id, char *recv_buf, size_t length){
    spi_transaction_t t;
    esp_err_t ret = -1;
    size_t transmit_length = length;
#if defined(CONFIG_IDF_TARGET_ESP32)||defined(CONFIG_IDF_TARGET_ESP32S2)||defined(CONFIG_IDF_TARGET_ESP32S3)
    if (spi_id > 3 || spi_id < 2){
        return -1;
    }
#else
    if (spi_id != 2){
        return -1;
    }
#endif
    char tmpbuff[SOC_SPI_MAXIMUM_BUFFER_SIZE];
    while (length > 0) {
        memset(&t, 0, sizeof(t));
        if ((spi_config[spi_id-2].flags & SPI_DEVICE_HALFDUPLEX) == 0){
            t.tx_buffer = tmpbuff;
            memset(tmpbuff, 0xFF, SOC_SPI_MAXIMUM_BUFFER_SIZE);
        }
        if (length > SOC_SPI_MAXIMUM_BUFFER_SIZE ) {
            if ((spi_config[spi_id-2].flags & SPI_DEVICE_HALFDUPLEX) == 0){
                t.length = SOC_SPI_MAXIMUM_BUFFER_SIZE  * 8;
            }
            t.rxlength = SOC_SPI_MAXIMUM_BUFFER_SIZE  * 8;
            t.rx_buffer = recv_buf;
            ret = spi_device_polling_transmit(spi_handle[spi_id-2], &t);
            recv_buf += SOC_SPI_MAXIMUM_BUFFER_SIZE ;
            length -= SOC_SPI_MAXIMUM_BUFFER_SIZE ;
        }
        else {
            t.rxlength = length * 8;
            t.rx_buffer = recv_buf;
            if ((spi_config[spi_id-2].flags & SPI_DEVICE_HALFDUPLEX) == 0){
                t.length = length * 8;
            }
            ret = spi_device_polling_transmit(spi_handle[spi_id-2], &t);
            break;
        }
    }
    return ret == 0 ? transmit_length : -1;
}

int luat_spi_send(int spi_id, const char *send_buf, size_t length){
    spi_transaction_t t;
    esp_err_t ret = -1;
    size_t transmit_length = length;
#if defined(CONFIG_IDF_TARGET_ESP32)||defined(CONFIG_IDF_TARGET_ESP32S2)||defined(CONFIG_IDF_TARGET_ESP32S3)
    if (spi_id > 3 || spi_id < 2){
        return -1;
    }
#else
    if (spi_id != 2){
        return -1;
    }
#endif
    char tmpbuff[SOC_SPI_MAXIMUM_BUFFER_SIZE];
    while (length > 0) {
        memset(&t, 0, sizeof(t));
        if ((spi_config[spi_id-2].flags & SPI_DEVICE_HALFDUPLEX) != 0){
            t.rx_buffer = tmpbuff;
            memset(tmpbuff, 0xFF, SOC_SPI_MAXIMUM_BUFFER_SIZE);
        }
        if (length > SOC_SPI_MAXIMUM_BUFFER_SIZE ) { 
            t.length = SOC_SPI_MAXIMUM_BUFFER_SIZE  * 8;
            t.tx_buffer = send_buf;
            if ((spi_config[spi_id-2].flags & SPI_DEVICE_HALFDUPLEX) != 0){
                t.rxlength = SOC_SPI_MAXIMUM_BUFFER_SIZE  * 8;
            }
            ret = spi_device_polling_transmit(spi_handle[spi_id-2], &t);
            send_buf += SOC_SPI_MAXIMUM_BUFFER_SIZE ;
            length -= SOC_SPI_MAXIMUM_BUFFER_SIZE ;
        }
        else {
            t.length = length * 8;
            t.tx_buffer = send_buf;
            if ((spi_config[spi_id-2].flags & SPI_DEVICE_HALFDUPLEX) != 0){
                t.rxlength = length  * 8;
            }
            ret = spi_device_polling_transmit(spi_handle[spi_id-2], &t);
            break;
        }
    }
    return ret == 0 ? transmit_length : -1;
}

#define LUAT_SPI_CS_SELECT 0
#define LUAT_SPI_CS_CLEAR 1

static uint8_t spi_bus[SOC_SPI_NUM] = {0};

int luat_spi_device_setup(luat_spi_device_t *spi_dev){
    esp_err_t ret = -1;
    int bus_id = spi_dev->bus_id;
    spi_device_handle_t *spi_device = luat_heap_malloc(sizeof(spi_device_handle_t));
    if (spi_device == NULL)
        return ret;
    spi_dev->user_data = (void *)spi_device;
#if defined(CONFIG_IDF_TARGET_ESP32)||defined(CONFIG_IDF_TARGET_ESP32S2)||defined(CONFIG_IDF_TARGET_ESP32S3)
    if (bus_id > 3 || bus_id < 2){
        return -1;
    }
#else
    if (bus_id != 2){
        return -1;
    }
#endif
    if (spi_bus[bus_id-2] == 0){
        spi_bus_config_t buscfg = {
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = SOC_SPI_MAXIMUM_BUFFER_SIZE
        };
        if (bus_id == 2){
            buscfg.miso_io_num = SPI2_MISO_IO_NUM;
            buscfg.mosi_io_num = SPI2_MOSI_IO_NUM;
            buscfg.sclk_io_num = SPI2_SCLK_IO_NUM;
        }
#if defined(CONFIG_IDF_TARGET_ESP32)||defined(CONFIG_IDF_TARGET_ESP32S2)||defined(CONFIG_IDF_TARGET_ESP32S3)
        else{
            buscfg.miso_io_num = SPI3_MISO_IO_NUM;
            buscfg.mosi_io_num = SPI3_MOSI_IO_NUM;
            buscfg.sclk_io_num = SPI3_SCLK_IO_NUM;
        }
#endif
        ret = spi_bus_initialize(bus_id-1, &buscfg, SPI_DMA_CH_AUTO);
        if (ret != 0){
            return ret;
        }
        spi_bus[bus_id-2] = 1;
    }
    spi_device_interface_config_t* dev_config = &spi_config[bus_id-2];
    memset(dev_config, 0, sizeof(spi_device_interface_config_t));
    if (spi_dev->spi_config.CPHA == 0){
        if (spi_dev->spi_config.CPOL == 0)
            dev_config->mode = 0;
        if (spi_dev->spi_config.CPOL == 1)
            dev_config->mode = 1;
    }else{
        if (spi_dev->spi_config.CPOL == 0)
            dev_config->mode = 2;
        if (spi_dev->spi_config.CPOL == 1)
            dev_config->mode = 3;
    }
    if (spi_dev->spi_config.mode == 0){
        dev_config->flags |= SPI_DEVICE_HALFDUPLEX;
    }
    dev_config->clock_speed_hz = spi_dev->spi_config.bandrate;
    dev_config->spics_io_num = -1; 
    dev_config->queue_size = 7;

    ret = spi_bus_add_device(bus_id-1, dev_config, spi_device);
    if (ret != 0)
        luat_heap_free(spi_device);
    luat_gpio_mode(spi_dev->spi_config.cs, Luat_GPIO_OUTPUT, Luat_GPIO_DEFAULT, Luat_GPIO_HIGH);
    return ret;
}

int luat_spi_device_close(luat_spi_device_t *spi_dev){
    esp_err_t ret = -1;
    int bus_id = spi_dev->bus_id;
#if defined(CONFIG_IDF_TARGET_ESP32)||defined(CONFIG_IDF_TARGET_ESP32S2)||defined(CONFIG_IDF_TARGET_ESP32S3)
    if (bus_id > 3 || bus_id < 2){
        return -1;
    }
#else
    if (bus_id != 2){
        return -1;
    }
#endif
    ret = spi_bus_remove_device(*(spi_device_handle_t *)(spi_dev->user_data));
    luat_heap_free((spi_device_handle_t *)(spi_dev->user_data));
    return ret;
}

int luat_spi_device_transfer(luat_spi_device_t *spi_dev, const char *send_buf, size_t send_length, char *recv_buf, size_t recv_length){
    if (spi_dev->spi_config.cs != 255)
        luat_gpio_set(spi_dev->spi_config.cs, LUAT_SPI_CS_SELECT);
    spi_transaction_t send = {0};
    spi_transaction_t recv = {0};
    esp_err_t ret = -1;
    size_t transmit_length = send_length;
    int bus_id = spi_dev->bus_id;
#if defined(CONFIG_IDF_TARGET_ESP32)||defined(CONFIG_IDF_TARGET_ESP32S2)||defined(CONFIG_IDF_TARGET_ESP32S3)
    if (bus_id > 3 || bus_id < 2){
        return -1;
    }
#else
    if (bus_id != 2){
        return -1;
    }
#endif
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
    recv.length = recv_length * 8;
    recv.rxlength = recv_length * 8;
    recv.rx_buffer = recv_buf;
    ret = spi_device_polling_transmit(*(spi_device_handle_t *)(spi_dev->user_data), &recv);
    if (spi_dev->spi_config.cs != 255)
        luat_gpio_set(spi_dev->spi_config.cs, LUAT_SPI_CS_CLEAR);
    return ret == 0 ? transmit_length : -1;
}

int luat_spi_device_recv(luat_spi_device_t *spi_dev, char *recv_buf, size_t length){
    if (spi_dev->spi_config.cs != 255)
        luat_gpio_set(spi_dev->spi_config.cs, LUAT_SPI_CS_SELECT);
    spi_transaction_t t = {0};
    esp_err_t ret = -1;
    size_t transmit_length = length;
    int bus_id = spi_dev->bus_id;
#if defined(CONFIG_IDF_TARGET_ESP32)||defined(CONFIG_IDF_TARGET_ESP32S2)||defined(CONFIG_IDF_TARGET_ESP32S3)
    if (bus_id > 3 || bus_id < 2){
        return -1;
    }
#else
    if (bus_id != 2){
        return -1;
    }
#endif
    bool is_full_duplex = spi_dev->spi_config.mode;
    char tmpbuff[SOC_SPI_MAXIMUM_BUFFER_SIZE];
    while (length > 0) {
        memset(&t, 0, sizeof(t));
        if (is_full_duplex) {
            t.tx_buffer = tmpbuff;
            memset(tmpbuff, 0xff, SOC_SPI_MAXIMUM_BUFFER_SIZE);
        }
        if (length > SOC_SPI_MAXIMUM_BUFFER_SIZE ) { 
            if (is_full_duplex) {
                t.length = SOC_SPI_MAXIMUM_BUFFER_SIZE  * 8;
            }
            t.rxlength = SOC_SPI_MAXIMUM_BUFFER_SIZE  * 8;
            t.rx_buffer = recv_buf;
            ret = spi_device_polling_transmit(*(spi_device_handle_t *)(spi_dev->user_data), &t);
            recv_buf += SOC_SPI_MAXIMUM_BUFFER_SIZE ;
            length -= SOC_SPI_MAXIMUM_BUFFER_SIZE ;
        }
        else {
            if (is_full_duplex) {
                t.length = length * 8;
            }
            t.rxlength = length * 8;
            t.rx_buffer = recv_buf;
            ret = spi_device_polling_transmit(*(spi_device_handle_t *)(spi_dev->user_data), &t);
            break;
        }
    }
    if (spi_dev->spi_config.cs != 255)
        luat_gpio_set(spi_dev->spi_config.cs, LUAT_SPI_CS_CLEAR);
    return ret == 0 ? transmit_length : -1;
}

int luat_spi_device_send(luat_spi_device_t *spi_dev, const char *send_buf, size_t length){
    if (spi_dev->spi_config.cs != 255)
        luat_gpio_set(spi_dev->spi_config.cs, LUAT_SPI_CS_SELECT);
    spi_transaction_t t = {0};
    esp_err_t ret = -1;
    size_t transmit_length = length;
    int bus_id = spi_dev->bus_id;
#if defined(CONFIG_IDF_TARGET_ESP32)||defined(CONFIG_IDF_TARGET_ESP32S2)||defined(CONFIG_IDF_TARGET_ESP32S3)
    if (bus_id > 3 || bus_id < 2){
        return -1;
    }
#else
    if (bus_id != 2){
        return -1;
    }
#endif
    char tmpbuff[SOC_SPI_MAXIMUM_BUFFER_SIZE];
    bool is_full_duplex = spi_dev->spi_config.mode;
    while (length > 0) {
        memset(&t, 0, sizeof(t));
        if (is_full_duplex) {
            t.rx_buffer = tmpbuff;
            memset(tmpbuff, 0xff, SOC_SPI_MAXIMUM_BUFFER_SIZE);
        }
        if (length > SOC_SPI_MAXIMUM_BUFFER_SIZE ) { 
            t.length = SOC_SPI_MAXIMUM_BUFFER_SIZE  * 8;
            t.tx_buffer = send_buf;
            if (is_full_duplex) {
                t.rxlength = SOC_SPI_MAXIMUM_BUFFER_SIZE  * 8;
            }
            ret = spi_device_polling_transmit(*(spi_device_handle_t *)(spi_dev->user_data), &t);
            send_buf += SOC_SPI_MAXIMUM_BUFFER_SIZE ;
            length -= SOC_SPI_MAXIMUM_BUFFER_SIZE ;
        }else {
            t.length = length * 8;
            t.tx_buffer = send_buf;
            if (is_full_duplex) {
                t.rxlength = length  * 8;
            }
            ret = spi_device_polling_transmit(*(spi_device_handle_t *)(spi_dev->user_data), &t);
            break;
        }
    }
    if (spi_dev->spi_config.cs != 255)
        luat_gpio_set(spi_dev->spi_config.cs, LUAT_SPI_CS_CLEAR);
    return ret == 0 ? transmit_length : -1;
}

int luat_spi_device_config(luat_spi_device_t* spi_dev){
    return 0;
}

int luat_spi_config_dma(int spi_id, uint32_t tx_channel, uint32_t rx_channel) {
    return 0;
}

int luat_spi_change_speed(int spi_id, uint32_t speed) {
    int ret = 0;
    spi_device_interface_config_t* dev_config = &spi_config[spi_id-2];
    dev_config->clock_speed_hz = speed;
    if (spi_handle[spi_id-2]) {
        ret = spi_bus_remove_device(spi_handle[spi_id-2]);
        // spi_handle[spi_id-2] = NULL;
    }
    ret =  spi_bus_add_device(spi_id-1, dev_config, &spi_handle[spi_id-2]);
    return ret;
    // return 0;
}

#ifdef LUAT_USE_FATFS
#include "ff.h"
#include "diskio.h"
void luat_spi_set_sdhc_ctrl(block_disk_t *disk) {

}

void luat_sdio_set_sdhc_ctrl(block_disk_t *disk) {
}
#endif
