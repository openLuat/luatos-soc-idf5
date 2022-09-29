idf_component_register(SRC_DIRS ${LUATOS_ROOT}/luat/modules
                            ${LUATOS_ROOT}/luat/vfs
                            ${LUATOS_ROOT}/lua/src
                            port
                            ${LUATOS_ROOT}/components/lfs
                            ${LUATOS_ROOT}/components/iotauth
                            ${LUATOS_ROOT}/components/lua-cjson
                            ${LUATOS_ROOT}/components/crypto
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
                    REQUIRES esp-tls lwip esp_http_client mbedtls spiffs
                    )

