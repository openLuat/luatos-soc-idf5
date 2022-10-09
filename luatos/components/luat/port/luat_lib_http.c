
/*
@module  http2
@summary http2客户端
@version 1.0
@date    2022.09.05
@demo network
*/

#include "luat_base.h"

#include "luat_rtos.h"
#include "luat_msgbus.h"
#include "luat_fs.h"
#include "luat_malloc.h"

#include "esp_tls.h"
#include "esp_http_client.h"

#define LUAT_LOG_TAG "http"
#include "luat_log.h"

#define HTTP_ERROR_OK 	    (0)
#define HTTP_ERROR_STATE 	(-1)
#define HTTP_ERROR_HEADER 	(-2)
#define HTTP_ERROR_BODY 	(-3)
#define HTTP_ERROR_CONNECT 	(-4)
#define HTTP_ERROR_CLOSE 	(-5)
#define HTTP_ERROR_RX 		(-6)

typedef struct http_header {
    char *key;   
    char *value; 
    struct http_header *next;
} http_header_t;

typedef struct{
    esp_http_client_handle_t http_client;
	const char *req_body;
	size_t req_body_len;
	char *dst;
	uint8_t is_download;
	// 响应相关
	http_header_t* resp_headers;//headers
    char *resp_buff;//body
	uint32_t resp_buff_len;
    uint32_t resp_buff_offset;
	FILE* fd;
	uint32_t fd_writed;
	uint8_t fd_ok;
	uint64_t idp;
	uint8_t close_state;
}luat_http_ctrl_t;

static int http_close(luat_http_ctrl_t *http_ctrl){
    if (http_ctrl->http_client) {
        esp_http_client_cleanup(http_ctrl->http_client);
        http_ctrl->http_client = NULL;
    }

    if (http_ctrl->is_download) {
        if (http_ctrl->fd != NULL) {
            luat_fs_fclose(http_ctrl->fd);
        }
    }
    if (http_ctrl->req_body != NULL) {
        luat_heap_free(http_ctrl->req_body);
        http_ctrl->req_body = NULL;
    }
    if (http_ctrl->dst != NULL) {
        luat_heap_free(http_ctrl->dst);
        http_ctrl->dst = NULL;
    }
    if (http_ctrl->resp_buff != NULL) {
        luat_heap_free(http_ctrl->resp_buff);
        http_ctrl->resp_buff = NULL;
    }
	luat_heap_free(http_ctrl);
	return 0;
}

// 这个是启动线程失败后, 主动回调cbcwait的中转函数
static int32_t l_http_cb_eaily(lua_State *L, void* ptr){
    rtos_msg_t* msg = (rtos_msg_t*)lua_topointer(L, -1);
    luat_http_ctrl_t *http_ctrl =(luat_http_ctrl_t *)msg->ptr;
    if (http_ctrl->idp) {
        lua_pushinteger(L, msg->arg1); // 把错误码返回去
	    luat_cbcwait(L, http_ctrl->idp, 1);
        // 只需要cwait回调一次
        http_ctrl->idp = 0;
    }
    http_close(http_ctrl);
    return 0;
}

static int32_t l_http_callback(lua_State *L, void* ptr){
    rtos_msg_t* msg = (rtos_msg_t*)lua_topointer(L, -1);
    luat_http_ctrl_t *http_ctrl =(luat_http_ctrl_t *)msg->ptr;
	uint64_t idp = http_ctrl->idp;

	// LLOGD("l_http_callback arg1:%d is_download:%d idp:%d",msg->arg1,http_ctrl->is_download,idp);
	if (msg->arg1){
		lua_pushinteger(L, msg->arg1); // 把错误码返回去
		luat_cbcwait(L, idp, 1);
		http_ctrl->idp = 0;
        http_close(http_ctrl);
		return 0;
	}

    lua_pushinteger(L, esp_http_client_get_status_code(http_ctrl->http_client));

    http_header_t* temp = http_ctrl->resp_headers;
    lua_newtable(L);
    while (temp){
        // LLOGD("resp_header key:%s value:%s",temp->key,temp->value);
    	lua_pushlstring(L, temp->key,strlen(temp->key));
        luat_heap_free(temp->key);
		lua_pushlstring(L, temp->value,strlen(temp->value));
        luat_heap_free(temp->value);
		lua_settable(L, -3);
        http_header_t* resp_header = temp;
        temp = (temp)->next;
        luat_heap_free(resp_header);
    }
    luat_heap_free(temp);

	// 处理body, 需要区分下载模式和非下载模式
	if (http_ctrl->is_download) {
        lua_pushinteger(L, luat_fs_fsize(http_ctrl->dst));
        luat_cbcwait(L, idp, 3); // code, headers, body
	} else {
		// 非下载模式
		lua_pushlstring(L, http_ctrl->resp_buff, http_ctrl->resp_buff_offset);
		luat_cbcwait(L, idp, 3); // code, headers, body
	}
	http_ctrl->idp = 0;
    http_close(http_ctrl);
	return 0;
}

static void http_resp_error(luat_http_ctrl_t *http_ctrl, int error_code) {
    //LLOGD("CALL http_resp_error");
	if (http_ctrl->close_state==0){
		http_ctrl->close_state=1;
		rtos_msg_t msg = {0};
		msg.handler = l_http_callback;
		msg.ptr = http_ctrl;
		msg.arg1 = error_code;
		luat_msgbus_put(&msg, 0);
	}
}

static esp_err_t l_http_event_handler(esp_http_client_event_t *evt) {
    luat_http_ctrl_t *http_ctrl = (luat_http_ctrl_t *)evt->user_data;
    http_header_t** temp = &(http_ctrl->resp_headers);
    int mbedtls_err = 0;
    esp_err_t err;
    LLOGD("http event %d", evt->event_id);
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            // LLOGD("HTTP_EVENT_ERROR");
            // http_resp_error(http_ctrl, HTTP_ERROR_CLOSE);
            break;
        case HTTP_EVENT_ON_CONNECTED:
            // LLOGD("HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            // LLOGD("HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            // LLOGD("HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            while (*temp){
                temp = &((*temp)->next);
            }
            *temp = (http_header_t *) luat_heap_malloc(sizeof(http_header_t));
            (*temp)->key = (char *) luat_heap_calloc(1,strlen(evt->header_key)+1);
            memcpy((*temp)->key, evt->header_key, strlen(evt->header_key));
            (*temp)->value = (char *) luat_heap_calloc(1,strlen(evt->header_value)+1);
            memcpy((*temp)->value, evt->header_value, strlen(evt->header_value));
            (*temp)->next = NULL;
            break;
        case HTTP_EVENT_ON_DATA:
            int status_code = esp_http_client_get_status_code(evt->client);
            if (status_code > 299) {
                return ESP_FAIL;
            }
            int content_length = esp_http_client_get_content_length(evt->client);
            // LLOGD("HTTP_EVENT_ON_DATA, len %d", evt->data_len);
            // LLOGD("content_length %d", content_length);
            if (evt->data_len > 0){
                if (http_ctrl->is_download) {
                    if (http_ctrl->fd == NULL) {
                        luat_fs_remove(http_ctrl->dst);
                        http_ctrl->fd = luat_fs_fopen(http_ctrl->dst, "w+");
                    }
                    if (http_ctrl->fd == NULL) {
                        LLOGE("open download file fail %s", http_ctrl->dst);
                        return ESP_FAIL;
                    }
                    luat_fs_fwrite(evt->data, evt->data_len, 1, http_ctrl->fd);
                }
                else {
                    if (http_ctrl->resp_buff == NULL) {
                        int malloc_size = evt->data_len;
                        if (content_length > 0)
                            malloc_size = content_length;
                        http_ctrl->resp_buff = (char *) luat_heap_malloc(malloc_size);
                        http_ctrl->resp_buff_len = malloc_size;
                        http_ctrl->resp_buff_offset = 0;
                    }
                    if (http_ctrl->resp_buff == NULL) {
                        LLOGE("Failed to malloc memory for http_ctrl->resp_buff");
                        return ESP_FAIL;
                    }
                    char* tmp = http_ctrl->resp_buff;
                    if (http_ctrl->resp_buff_offset + evt->data_len > http_ctrl->resp_buff_len) {
                        // LLOGD("realloc buff %d", http_ctrl->resp_buff_offset + evt->data_len);
                        tmp = luat_heap_realloc(http_ctrl->resp_buff, http_ctrl->resp_buff_offset + evt->data_len);
                        if (tmp == NULL) {
                            LLOGE("Failed to malloc memory for http_ctrl->resp_buff");
                            return ESP_FAIL;
                        }
                        http_ctrl->resp_buff = tmp;
                    }
                    memcpy(http_ctrl->resp_buff + http_ctrl->resp_buff_offset, evt->data, evt->data_len);
                    http_ctrl->resp_buff_offset += evt->data_len;
                }
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            // LLOGD("HTTP_EVENT_ON_FINISH");
            
            // http_resp_error(http_ctrl, HTTP_ERROR_OK);
            break;
        case HTTP_EVENT_DISCONNECTED:
            // LLOGI("HTTP_EVENT_DISCONNECTED");
            // err = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err, NULL);
            // if (err != 0) {
            //     LLOGI("Last esp error code: 0x%x", err);
            //     LLOGI("Last mbedtls failure: 0x%x", mbedtls_err);
            // }
            // http_resp_error(http_ctrl, HTTP_ERROR_CLOSE);
            break;
        case HTTP_EVENT_REDIRECT:
            // LLOGI("HTTP_EVENT_REDIRECT");
            break;
    }
    return ESP_OK;
}

static void luat_http_task_entry(void* arg) {
    luat_http_ctrl_t *http_ctrl = (luat_http_ctrl_t *)arg;
    esp_err_t err = esp_http_client_perform(http_ctrl->http_client);
    LLOGD("esp_http_client_perform %d", err);
    if (err){
        http_resp_error(http_ctrl, -1);
    }
    else {
        http_resp_error(http_ctrl, HTTP_ERROR_OK);
    }
    vTaskDelete(NULL);
}


/*
http2客户端
@api http2.request(method,url,headers,body,opts,ca_file)
@string 请求方法, 支持 GET/POST
@string url地址
@tabal  请求头 可选 例如{["Content-Type"] = "application/x-www-form-urlencoded"}
@string body 可选
@tabal  额外配置 可选 包含dst:下载路径,可选 adapter:选择使用网卡,可选
@string 证书 可选
@return int code
@return tabal headers 
@return string body
@usage 
local code, headers, body = http2.request("GET","http://site0.cn/api/httptest/simple/time").wait()
log.info("http2.get", code, headers, body)
*/
static int l_http_request(lua_State *L) {
    esp_err_t err = 0;
	size_t len = 0;
    esp_http_client_config_t http_conf = {0};
    luat_http_ctrl_t *http_ctrl = (luat_http_ctrl_t *)luat_heap_malloc(sizeof(luat_http_ctrl_t));
    if (!http_ctrl){
		LLOGE("out of memory when malloc http_ctrl");
		luat_pushcwait_error(L,HTTP_ERROR_CONNECT);
		return 1;
	}
	memset(http_ctrl, 0, sizeof(luat_http_ctrl_t));
    http_ctrl->resp_headers = NULL;
    const char *method = luaL_optlstring(L, 1, "GET", &len);
	if (!strncmp("GET", method, strlen("GET"))) {
        http_conf.method = HTTP_METHOD_GET;
        LLOGI("HTTP GET");
    }
    else if (!strncmp("POST", method, strlen("POST"))) {
        http_conf.method = HTTP_METHOD_POST;
        LLOGI("HTTP POST");
    }
    else if (!strncmp("PUT", method, strlen("PUT"))) {
        http_conf.method = HTTP_METHOD_PUT;
        LLOGI("HTTP PUT");
    }
    else if (!strncmp("DELETE", method, strlen("DELETE"))) {
        http_conf.method = HTTP_METHOD_DELETE;
        LLOGI("HTTP DELETE");
    }
    else {
        LLOGI("only GET/POST supported %s", method);
        goto error;
    }

    http_conf.url = luaL_checkstring(L, 2);
    http_conf.event_handler = l_http_event_handler;
    http_conf.keep_alive_enable = false;
    http_conf.user_data = (void*)http_ctrl;
    http_conf.disable_auto_redirect = true;
    http_ctrl->http_client = esp_http_client_init(&http_conf);
    
	if (lua_istable(L, 3)) {
		lua_pushnil(L);
		while (lua_next(L, 3) != 0) {
			const char *name = lua_tostring(L, -2);
			const char *value = lua_tostring(L, -1);
            err = esp_http_client_set_header(http_ctrl->http_client, name, value);
            if (err){
                LLOGI("esp_http_client_set_header error:%d",err);
                goto error;
            }
			lua_pop(L, 1);
		}
	}
	if (lua_isstring(L, 4)) {
		const char *body = luaL_checklstring(L, 4, &(http_ctrl->req_body_len));
		http_ctrl->req_body = luat_heap_malloc((http_ctrl->req_body_len) + 1);
		memset(http_ctrl->req_body, 0, (http_ctrl->req_body_len) + 1);
		memcpy(http_ctrl->req_body, body, (http_ctrl->req_body_len));
        err = esp_http_client_set_post_field(http_ctrl->http_client, http_ctrl->req_body, http_ctrl->req_body_len);
        if (err){
            LLOGI("esp_http_client_set_post_field error:%d",err);
            goto error;
        }
    }

	if (lua_istable(L, 5)){
		lua_pushstring(L, "timeout");
		if (LUA_TNUMBER == lua_gettable(L, 5)) {
            err = esp_http_client_set_timeout_ms(http_ctrl->http_client, luaL_optinteger(L, -1, 0));
            if (err){
                LLOGI("esp_http_client_set_timeout_ms error:%d",err);
                goto error;
            }
		}
		lua_pop(L, 1);

		lua_pushstring(L, "dst");
		if (LUA_TSTRING == lua_gettable(L, 5)) {
			const char *dst = luaL_checklstring(L, -1, &len);
			http_ctrl->dst = luat_heap_malloc(len + 1);
			memset(http_ctrl->dst, 0, len + 1);
			memcpy(http_ctrl->dst, dst, len);
			http_ctrl->is_download = 1;
		}
		lua_pop(L, 1);
	}

    http_ctrl->idp = luat_pushcwait(L);
    if (pdPASS != xTaskCreate(luat_http_task_entry, "http", 16*1024, (void*)http_ctrl, tskIDLE_PRIORITY + 5, NULL)) {
        goto error;
    }
    return 1;
error:
	http_close(http_ctrl);
	luat_pushcwait_error(L,HTTP_ERROR_CONNECT);
	return 1;
}


#include "rotable2.h"
static const rotable_Reg_t reg_http[] =
{
	{"request",			ROREG_FUNC(l_http_request)},
	{ NULL,             ROREG_INT(0)}
};

int luaopen_http( lua_State *L ) {
    luat_newlib2(L, reg_http);
    return 1;
}
