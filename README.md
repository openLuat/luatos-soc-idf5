# 基于IDF5适配LuatOS

本库的起始版本号是V1001, 部分API与V000x系列不兼容,请查阅最新demo

## 支持状态

* 芯片支持 ESP32C3/ESP32C2/ESP32
* 库支持: 所有基础外设,wifi,ble,http2,mqtt,httpsrv. 当前尚未支持socket/libnet

## 技术支持和文档

主文档: https://wiki.luatos.com
demo地址: https://gitee.com/openLuat/LuatOS/tree/master/demo/wlan/esp32c3

## 编译说明

* 本代码库需要idf5最新代码进行编译, 5.0-beta1会报错
* idf5目录尚未更新到最新版, 直接使用会报错, 当前主要是为了方便看源码,后续可能删除.

## 开源协议

MIT License
