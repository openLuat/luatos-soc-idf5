/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "blehr_sens.h"

#include "luat_base.h"
#include "luat_msgbus.h"
#include "luat_malloc.h"
#include "luat_nimble.h"

static const char *manuf_name = "LuatOS";
static const char *model_num = "BLE Demo";
uint16_t hrs_hrm_handle;
uint16_t g_ble_attr_indicate_handle;
uint16_t g_ble_attr_write_handle;
uint16_t g_ble_conn_handle;
extern uint16_t g_ble_state;

#define WM_GATT_SVC_UUID      0xFFF0
#define WM_GATT_INDICATE_UUID 0xFFF1
#define WM_GATT_WRITE_UUID    0xFFF2
#define WM_GATT_NOTIFY_UUID    0xFFF3

#define LUAT_LOG_TAG "nimble"
#include "luat_log.h"

typedef struct ble_write_msg {
    // uint16_t conn_handle,
    // uint16_t attr_handle,
    ble_uuid_t* uuid;
    uint16_t len;
    char buff[1];
}ble_write_msg_t;

static int
gatt_svr_chr_access_heart_rate(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg);

static int
gatt_svr_chr_access_device_info(uint16_t conn_handle, uint16_t attr_handle,
                                struct ble_gatt_access_ctxt *ctxt, void *arg);

static int
gatt_svr_chr_access_func(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg);

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        /* Service: Heart-rate */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(GATT_HRS_UUID),
        .characteristics = (struct ble_gatt_chr_def[])
        { {
#if 0
                /* Characteristic: Heart-rate measurement */
                .uuid = BLE_UUID16_DECLARE(GATT_HRS_MEASUREMENT_UUID),
                .access_cb = gatt_svr_chr_access_heart_rate,
                .val_handle = &hrs_hrm_handle,
                .flags = BLE_GATT_CHR_F_NOTIFY,
            }, {
                /* Characteristic: Body sensor location */
                .uuid = BLE_UUID16_DECLARE(GATT_HRS_BODY_SENSOR_LOC_UUID),
                .access_cb = gatt_svr_chr_access_heart_rate,
                .flags = BLE_GATT_CHR_F_READ,
            }, {
#endif
                /* Characteristic: Body sensor location */
                .uuid = BLE_UUID16_DECLARE(WM_GATT_WRITE_UUID),
                .val_handle = &g_ble_attr_write_handle,
                .access_cb = gatt_svr_chr_access_func,
                .flags = BLE_GATT_CHR_F_WRITE,
            }, {
                /* Characteristic: Body sensor location */
                .uuid = BLE_UUID16_DECLARE(WM_GATT_INDICATE_UUID),
                .val_handle = &g_ble_attr_indicate_handle,
                .access_cb = gatt_svr_chr_access_func,
                .flags = BLE_GATT_CHR_F_INDICATE | BLE_GATT_CHR_F_READ,
            }, {
                0, /* No more characteristics in this service */
            },
        }
    },
#if 1
    {
        /* Service: Device Information */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(GATT_DEVICE_INFO_UUID),
        .characteristics = (struct ble_gatt_chr_def[])
        { {
                /* Characteristic: * Manufacturer name */
                .uuid = BLE_UUID16_DECLARE(GATT_MANUFACTURER_NAME_UUID),
                .access_cb = gatt_svr_chr_access_device_info,
                .flags = BLE_GATT_CHR_F_READ,
            }, {
                /* Characteristic: Model number string */
                .uuid = BLE_UUID16_DECLARE(GATT_MODEL_NUMBER_UUID),
                .access_cb = gatt_svr_chr_access_device_info,
                .flags = BLE_GATT_CHR_F_READ,
            }, {
                0, /* No more characteristics in this service */
            },
        }
    },
#endif
    {
        0, /* No more services */
    },
};

static int
gatt_svr_chr_access_heart_rate(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    /* Sensor location, set to "Chest" */
    static uint8_t body_sens_loc = 0x01;
    uint16_t uuid;
    int rc;

    uuid = ble_uuid_u16(ctxt->chr->uuid);

    if (uuid == GATT_HRS_BODY_SENSOR_LOC_UUID) {
        rc = os_mbuf_append(ctxt->om, &body_sens_loc, sizeof(body_sens_loc));

        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    assert(0);
    return BLE_ATT_ERR_UNLIKELY;
}

static int
gatt_svr_chr_access_device_info(uint16_t conn_handle, uint16_t attr_handle,
                                struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    uint16_t uuid;
    int rc;

    uuid = ble_uuid_u16(ctxt->chr->uuid);

    if (uuid == GATT_MODEL_NUMBER_UUID) {
        rc = os_mbuf_append(ctxt->om, model_num, strlen(model_num));
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    if (uuid == GATT_MANUFACTURER_NAME_UUID) {
        rc = os_mbuf_append(ctxt->om, manuf_name, strlen(manuf_name));
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    assert(0);
    return BLE_ATT_ERR_UNLIKELY;
}

void
gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg)
{
    char buf[BLE_UUID_STR_LEN];

    switch (ctxt->op) {
    case BLE_GATT_REGISTER_OP_SVC:
        LLOGD("registered service %s with handle=%d\n",
                    ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
                    ctxt->svc.handle);
        break;

    case BLE_GATT_REGISTER_OP_CHR:
        LLOGD("registering characteristic %s with "
                    "def_handle=%d val_handle=%d\n",
                    ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
                    ctxt->chr.def_handle,
                    ctxt->chr.val_handle);
        break;

    case BLE_GATT_REGISTER_OP_DSC:
        LLOGD("registering descriptor %s with handle=%d\n",
                    ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
                    ctxt->dsc.handle);
        break;

    default:
        assert(0);
        break;
    }
}

static int l_ble_chr_write_cb(lua_State* L, void* ptr) {
    ble_write_msg_t* wmsg = (ble_write_msg_t*)ptr;
    rtos_msg_t* msg = (rtos_msg_t*)lua_topointer(L, -1);
    lua_getglobal(L, "sys_pub");
    if (lua_isfunction(L, -1)) {
        lua_pushstring(L, "BLE_GATT_WRITE_CHR");
        lua_newtable(L);
        lua_pushlstring(L, wmsg->buff, wmsg->len);
        lua_call(L, 3, 0);
    }
    luat_heap_free(wmsg);
    return 0;
}

static int
gatt_svr_chr_access_func(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    int i = 0;
    struct os_mbuf *om = ctxt->om;
    ble_write_msg_t* wmsg;
    rtos_msg_t msg = {0};
    LLOGD("gatt_svr_chr_access_func %d %d %d", conn_handle, attr_handle, ctxt->op);
    switch (ctxt->op) {
        case BLE_GATT_ACCESS_OP_WRITE_CHR:
              while(om) {
                  wmsg = (ble_write_msg_t*)(luat_heap_malloc(sizeof(ble_write_msg_t) + om->om_len - 1));
                  if (wmsg != NULL) {
                      wmsg->len = om->om_len;
                      msg.handler = l_ble_chr_write_cb;
                      msg.ptr = wmsg;
                      msg.arg1 = conn_handle;
                      msg.arg2 = attr_handle;
                      memcpy(wmsg->buff, om->om_data, om->om_len);
                      luat_msgbus_put(&msg, 0);
                  }
                //   if(g_ble_uart_output_fptr)
                //   {
                //     g_ble_uart_output_fptr((uint8_t *)om->om_data, om->om_len);
                    
                //   }else
                //   {
                //      print_bytes(om->om_data, om->om_len); 
                //   }
                  om = SLIST_NEXT(om, om_next);
              }
              return 0;
        case BLE_GATT_ACCESS_OP_READ_CHR:
            return 0;
        default:
            assert(0);
            return BLE_ATT_ERR_UNLIKELY;
    }
}

int gatt_svr_init(void)
{
    int rc;

    // ble_svc_gap_init();
    //ble_gatts_reset();
    // ble_svc_gatt_init();

    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }
    return 0;
}

int luat_nimble_server_send(int id, char* data, size_t data_len) {
    int rc;
    struct os_mbuf *om;

    if (g_ble_state != BT_STATE_CONNECTED) {
        LLOGI("Not connected yet");
        return -1;
    }

    om = ble_hs_mbuf_from_flat((const void*)data, (uint16_t)data_len);
    if (!om) {
        LLOGE("ble_hs_mbuf_from_flat return NULL!!");
        return BLE_HS_ENOMEM;
    }
    rc = ble_gattc_indicate_custom(g_ble_conn_handle,g_ble_attr_indicate_handle, om);
    LLOGD("ble_gattc_indicate_custom ret %d", rc);
    return 0;
}
