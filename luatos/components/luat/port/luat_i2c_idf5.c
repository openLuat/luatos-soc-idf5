
#include "luat_base.h"
#include "luat_i2c.h"

#include "driver/i2c.h"

#include "idf5_io_def.h"

#define LUAT_LOG_TAG "i2c"
#include "luat_log.h"

#define ACK_CHECK_EN 0x1           /*!< I2C master will check ack from slave*/

#define I2C_CHECK(i2cid) ((i2cid<0 || i2cid>=SOC_I2C_NUM) ? -1:0)

int luat_i2c_exist(int id){
    return (I2C_CHECK(id)==0);
}

int luat_i2c_setup(int id, int speed){
    if (I2C_CHECK(id)){
        return -1;
    }
    i2c_config_t conf = {0};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_SDA_IO_NUM;
    conf.scl_io_num = I2C_SCL_IO_NUM;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    if (speed == 0)
        conf.master.clk_speed = 100 * 1000;
    else if (speed == 1)
        conf.master.clk_speed = 400 * 1000;
    i2c_param_config(id, &conf);
    i2c_driver_install(id, conf.mode, 0, 0, 0);
    return 0;
}

int luat_i2c_close(int id){
    if (I2C_CHECK(id)){
        return -1;
    }
    i2c_driver_delete(id);
    return 0;
}

int luat_i2c_send(int id, int addr, void *buff, size_t len, uint8_t stop){
    if (I2C_CHECK(id)){
        return -1;
    }
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, addr << 1 | I2C_MASTER_WRITE, ACK_CHECK_EN);
    i2c_master_write(cmd, (const uint8_t *)buff, len, ACK_CHECK_EN);
    if (stop)
        i2c_master_stop(cmd);
    i2c_master_cmd_begin(id, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return 0;
}

int luat_i2c_recv(int id, int addr, void *buff, size_t len){
    if (I2C_CHECK(id)){
        return -1;
    }
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, addr << 1 | I2C_MASTER_READ, ACK_CHECK_EN);
    i2c_master_read(cmd, (uint8_t *)buff, len, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(id, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return 0;
}
