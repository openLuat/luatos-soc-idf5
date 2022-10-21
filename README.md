# 基于IDF5适配LuatOS

本库的起始版本号是V1001, 部分API与V000x系列不兼容,请查阅最新demo

## 支持状态

* 芯片支持 ESP32C3/ESP32C2/ESP32
* 库支持: 所有基础外设,wifi,ble,http2,mqtt,httpsrv. 当前尚未支持socket/libnet

## 技术支持和文档

主文档: https://wiki.luatos.com
demo地址: https://gitee.com/openLuat/LuatOS/tree/master/demo/wlan/esp32c3

## 编译说明

本代码库需要idf5最新代码进行编译, 且**依赖LuatOS主库**. 

主库不是当前库的luatos目录, 是另一个git库!! https://gitee.com/openLuat/LuatOS

* 首先, 将[LuatOS主库](https://gitee.com/openLuat/LuatOS)下载/clone下来
* 然后, 修改本库的luatos目录下的`CMakeLists.txt`, 里面有 LUATOS_ROOT, 修改成主库的本地完整路径

最终的目录结构类似这样:
```
D:\
    gitee\
        - LuatOS     # 这是主库的代码
            - lua
            - luat
            - 其他目录未列出
        - luatos-soc-idf5
            - idf5
            - luatos # 这是适配LuatOS的代码,不是主库
```

### 关于工具链

要用CMD, 不要用PowerShell!!!

方案1 : 使用本代码库自带的idf5

* idf5目录当前更新 2022.10.18 master分支
* 第一次使用需要初始化工具链

```log
cd idf5
install.bat
export.bat
```
方案2 :
* 自行clone idf5, 并初始化子目录及执行install.bat

### 编译操作

```log
cd luatos
# 第一次编译需要编译目标, 例如 esp32 esp32c2 esp32c3
idf.py set-target esp32c3 
# 执行编译
idf.py build
```

编译完成后, 会生成soc文件, 使用luatools刷机即可

若提示目录不匹配, 可以执行完整清理

```
idf.py fullclean
```


## 开源协议

MIT License
