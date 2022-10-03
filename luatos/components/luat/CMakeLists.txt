idf_component_register(SRC_DIRS ${LUATOS_ROOT}/luat/modules
                                ${LUATOS_ROOT}/luat/vfs
                                ${LUATOS_ROOT}/lua/src
                                port
                                ${LUATOS_ROOT}/components/lfs
                                ${LUATOS_ROOT}/components/iotauth
                                ${LUATOS_ROOT}/components/lua-cjson
                                ${LUATOS_ROOT}/components/crypto
                                ${LUATOS_ROOT}/components/miniz
                                ${LUATOS_ROOT}/components/serialization/protobuf
                                ${LUATOS_ROOT}/components/coremark
                                ${LUATOS_ROOT}/components/fal/src
                                ${LUATOS_ROOT}/components/flashdb/src
                                ${LUATOS_ROOT}/components/minmea
                    EXCLUDE_SRCS
                                ${LUATOS_ROOT}/luat/modules/luat_lib_http.c
                    INCLUDE_DIRS ../../include
                                ${LUATOS_ROOT}/lua/include
                                ${LUATOS_ROOT}/luat/include
                                ${LUATOS_ROOT}/components/cmux
                                ${LUATOS_ROOT}/components/fatfs
                                ${LUATOS_ROOT}/components/lfs
                                ${LUATOS_ROOT}/components/lua-cjson
                                ${LUATOS_ROOT}/components/crypto
                                ${LUATOS_ROOT}/components/miniz
                                ${LUATOS_ROOT}/components/serialization/protobuf
                                ${LUATOS_ROOT}/components/coremark
                                ${LUATOS_ROOT}/components/fal/inc
                                ${LUATOS_ROOT}/components/flashdb/inc
                                ${LUATOS_ROOT}/components/minmea
                    REQUIRES esp-tls lwip esp_http_client mbedtls spiffs driver heap
                    )

