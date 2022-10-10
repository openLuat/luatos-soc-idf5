/*
@module  mqtt
@summary mqtt客户端
@version 1.0
@date    2022.08.25
@demo network
*/

#include "luat_base.h"
#include "luat_rtos.h"
#include "luat_zbuff.h"
#include "luat_malloc.h"

#include "mqtt_client.h"

#define LUAT_LOG_TAG "mqtt"
#include "luat_log.h"

#define LUAT_MQTT_CTRL_TYPE "MQTTCTRL*"
#define MQTT_MSG_RELEASE 0

typedef struct{
	esp_mqtt_client_config_t mqtt_cfg;
	esp_mqtt_client_handle_t client;
	int mqtt_cb;				// mqtt lua回调函数
	uint8_t adapter_index; 		// 适配器索引号, 似乎并没有什么用
	uint8_t mqtt_state;    		// mqtt状态
	uint8_t reconnect;    		// mqtt是否重连
	uint32_t reconnect_time;    // mqtt重连时间 单位ms
	void* reconnect_timer;		// mqtt重连定时器
	int mqtt_ref;				// 强制引用自身避免被GC
}luat_mqtt_ctrl_t;

typedef struct{
	uint16_t topic_len;
    uint16_t payload_len;
	uint8_t data[];
}luat_mqtt_msg_t;

static void mqtt_close(luat_mqtt_ctrl_t *mqtt_ctrl);
static int32_t l_mqtt_callback(lua_State *L, void* ptr);

static luat_mqtt_ctrl_t * get_mqtt_ctrl(lua_State *L){
	if (luaL_testudata(L, 1, LUAT_MQTT_CTRL_TYPE)){
		return ((luat_mqtt_ctrl_t *)luaL_checkudata(L, 1, LUAT_MQTT_CTRL_TYPE));
	}else{
		return ((luat_mqtt_ctrl_t *)lua_touserdata(L, 1));
	}
	return 0;
}

static void reconnect_timer_cb(void *data, void *param){
	luat_mqtt_ctrl_t * mqtt_ctrl = (luat_mqtt_ctrl_t *)param;
	esp_mqtt_client_reconnect(mqtt_ctrl->client);
}

static void mqtt_close(luat_mqtt_ctrl_t *mqtt_ctrl){
	mqtt_ctrl->mqtt_state = 0;
	if (mqtt_ctrl->reconnect){
		mqtt_ctrl->reconnect_timer = luat_create_rtos_timer(reconnect_timer_cb, mqtt_ctrl, NULL);
		luat_start_rtos_timer(mqtt_ctrl->reconnect_timer, mqtt_ctrl->reconnect_time, 0);
	}
}

static void mqtt_release(luat_mqtt_ctrl_t *mqtt_ctrl){
	rtos_msg_t msg = {0};
	msg.handler = l_mqtt_callback;
	msg.ptr = mqtt_ctrl;
	msg.arg1 = MQTT_MSG_RELEASE;
	luat_msgbus_put(&msg, 0);
}

static int32_t l_mqtt_callback(lua_State *L, void* ptr){
    rtos_msg_t* msg = (rtos_msg_t*)lua_topointer(L, -1);
    luat_mqtt_ctrl_t *mqtt_ctrl =(luat_mqtt_ctrl_t *)msg->ptr;
    switch (msg->arg1) {
		case MQTT_EVENT_DATA : {
			luat_mqtt_msg_t *mqtt_msg =(luat_mqtt_msg_t *)msg->arg2;
			if (mqtt_ctrl->mqtt_cb) {
				luat_mqtt_msg_t *mqtt_msg =(luat_mqtt_msg_t *)msg->arg2;
				lua_geti(L, LUA_REGISTRYINDEX, mqtt_ctrl->mqtt_cb);
				if (lua_isfunction(L, -1)) {
					lua_geti(L, LUA_REGISTRYINDEX, mqtt_ctrl->mqtt_ref);
					lua_pushstring(L, "recv");
					lua_pushlstring(L, (char *)(mqtt_msg->data),mqtt_msg->topic_len);
					lua_pushlstring(L, (char *)(mqtt_msg->data+mqtt_msg->topic_len),mqtt_msg->payload_len);
					lua_call(L, 4, 0);
				}
            }
			luat_heap_free(mqtt_msg);
            break;
        }
        case MQTT_EVENT_CONNECTED: {
			if (mqtt_ctrl->mqtt_cb) {
				lua_geti(L, LUA_REGISTRYINDEX, mqtt_ctrl->mqtt_cb);
				if (lua_isfunction(L, -1)) {
					lua_geti(L, LUA_REGISTRYINDEX, mqtt_ctrl->mqtt_ref);
					lua_pushstring(L, "conack");
					lua_call(L, 2, 0);
				}
				lua_getglobal(L, "sys_pub");
				if (lua_isfunction(L, -1)) {
					lua_pushstring(L, "MQTT_CONNACK");
					lua_geti(L, LUA_REGISTRYINDEX, mqtt_ctrl->mqtt_ref);
					lua_call(L, 2, 0);
				}
            }
            break;
        }
		case MQTT_EVENT_PUBLISHED: {
			if (mqtt_ctrl->mqtt_cb) {
				lua_geti(L, LUA_REGISTRYINDEX, mqtt_ctrl->mqtt_cb);
				if (lua_isfunction(L, -1)) {
					lua_geti(L, LUA_REGISTRYINDEX, mqtt_ctrl->mqtt_ref);
					lua_pushstring(L, "sent");
					lua_pushinteger(L, msg->arg2);
					lua_call(L, 3, 0);
				}
            }
            break;
        }
		case MQTT_MSG_RELEASE: {
			if (mqtt_ctrl->mqtt_ref) {
				luaL_unref(L, LUA_REGISTRYINDEX, mqtt_ctrl->mqtt_ref);
				mqtt_ctrl->mqtt_ref = 0;
            }
            break;
        }
		default : {
			LLOGD("l_mqtt_callback error arg1:%d",msg->arg1);
            break;
        }
    }
    lua_pushinteger(L, 0);
    return 1;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data){
	luat_mqtt_ctrl_t* mqtt_ctrl = handler_args;
	esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
	rtos_msg_t msg = {
		.handler = l_mqtt_callback,
		.ptr = mqtt_ctrl,
		.arg1 = (esp_mqtt_event_id_t)event_id
	};
    switch ((esp_mqtt_event_id_t)event_id) {
		case MQTT_EVENT_CONNECTED:
			// LLOGI("MQTT_EVENT_CONNECTED");
			mqtt_ctrl->mqtt_state = 1;
			luat_msgbus_put(&msg, 0);
			break;
		case MQTT_EVENT_DISCONNECTED:
			mqtt_close(mqtt_ctrl);
			// LLOGI("MQTT_EVENT_DISCONNECTED");
			break;

		case MQTT_EVENT_SUBSCRIBED:
			// LLOGI("MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
			break;
		case MQTT_EVENT_UNSUBSCRIBED:
			// LLOGI("MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
			break;
		case MQTT_EVENT_PUBLISHED:
			// LLOGI("MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
			msg.arg2 = event->msg_id;
			luat_msgbus_put(&msg, 0);
			break;
		case MQTT_EVENT_DATA:
			// LLOGI("MQTT_EVENT_DATA");
			luat_mqtt_msg_t *mqtt_msg = (luat_mqtt_msg_t *)luat_heap_malloc(sizeof(luat_mqtt_msg_t)+event->topic_len+event->data_len);
			memcpy(mqtt_msg->data, event->topic, event->topic_len);
			memcpy(mqtt_msg->data+event->topic_len, event->data, event->data_len);
			mqtt_msg->topic_len = event->topic_len;
			mqtt_msg->payload_len = event->data_len;
			msg.arg2 = mqtt_msg;
			luat_msgbus_put(&msg, 0);
			break;
		case MQTT_EVENT_ERROR:
			mqtt_close(mqtt_ctrl);
			break;
		default:
			LLOGW("Other event id:%d", event->event_id);
			break;
    }
}

/*
订阅主题
@api mqttc:subscribe(topic, qos)
@string/table 主题
@int topic为string时生效 0/1/2 默认0
@usage 
mqttc:subscribe("/luatos/123456")
mqttc:subscribe({["/luatos/1234567"]=1,["/luatos/12345678"]=2})
*/
static int l_mqtt_subscribe(lua_State *L) {
	size_t len = 0;
	luat_mqtt_ctrl_t * mqtt_ctrl = (luat_mqtt_ctrl_t *)lua_touserdata(L, 1);
	if (lua_isstring(L, 2)){
		const char * topic = luaL_checklstring(L, 2, &len);
		uint8_t qos = luaL_optinteger(L, 3, 0);
		esp_mqtt_client_subscribe(mqtt_ctrl->client, topic, qos);
	}else if(lua_istable(L, 2)){
		lua_pushnil(L);
		while (lua_next(L, 2) != 0) {
			esp_mqtt_client_subscribe(mqtt_ctrl->client, lua_tostring(L, -2), luaL_optinteger(L, -1, 0));
			lua_pop(L, 1);
		}
	}
	return 0;
}

/*
取消订阅主题
@api mqttc:unsubscribe(topic)
@string/table 主题
@usage 
mqttc:unsubscribe("/luatos/123456")
mqttc:unsubscribe({"/luatos/1234567","/luatos/12345678"})
*/
static int l_mqtt_unsubscribe(lua_State *L) {
	size_t len = 0;
	luat_mqtt_ctrl_t * mqtt_ctrl = (luat_mqtt_ctrl_t *)lua_touserdata(L, 1);
	if (lua_isstring(L, 2)){
		const char * topic = luaL_checklstring(L, 2, &len);
		esp_mqtt_client_unsubscribe(mqtt_ctrl->client, topic);
	}else if(lua_istable(L, 2)){
		size_t count = lua_rawlen(L, 2);
		for (size_t i = 1; i <= count; i++){
			lua_geti(L, 2, i);
			const char * topic = luaL_checklstring(L, -1, &len);
			esp_mqtt_client_unsubscribe(mqtt_ctrl->client, topic);
			lua_pop(L, 1);
		}
	}
	return 0;
}

/*
mqtt客户端创建
@api mqttc:create(adapter,host,port,isssl,ca_file)
@int 适配器序号, 只能是network.ETH0,network.STA,network.AP,如果不填,会选择最后一个注册的适配器
@string 服务器地址
@int  	端口号
@bool  	是否为ssl加密连接,默认不加密
@string 证书
@usage 
mqttc = mqtt.create(nil,"120.55.137.106", 1884)
*/
static int l_mqtt_create(lua_State *L) {
	size_t client_cert_len, client_key_len, client_password_len;
	const char *client_cert = NULL;
	const char *client_key = NULL;
	const char *client_password = NULL;
	int adapter_index = luaL_optinteger(L, 1, 0);
	size_t ip_len = 0;
	luat_mqtt_ctrl_t *mqtt_ctrl = (luat_mqtt_ctrl_t *)lua_newuserdata(L, sizeof(luat_mqtt_ctrl_t));
	if (!mqtt_ctrl){
		return 0;
	}
	memset(mqtt_ctrl, 0, sizeof(luat_mqtt_ctrl_t));
	mqtt_ctrl->adapter_index = adapter_index;

	mqtt_ctrl->mqtt_cfg.broker.address.uri = luaL_checklstring(L, 2, &ip_len);
	mqtt_ctrl->mqtt_cfg.session.keepalive = 240;

	if (lua_isnumber(L, 3)){
		mqtt_ctrl->mqtt_cfg.broker.address.port = luaL_checkinteger(L, 3);
	}
	mqtt_ctrl->mqtt_state = 0;
	
	luaL_setmetatable(L, LUAT_MQTT_CTRL_TYPE);
	lua_pushvalue(L, -1);
	mqtt_ctrl->mqtt_ref = luaL_ref(L, LUA_REGISTRYINDEX);
	return 1;
}

/*
mqtt三元组配置
@api mqttc:auth(client_id,username,password)
@string client_id
@string 账号 可选
@string 密码 可选
@usage 
mqttc:auth("123456789","username","password")
*/
static int l_mqtt_auth(lua_State *L) {
	luat_mqtt_ctrl_t * mqtt_ctrl = get_mqtt_ctrl(L);
	mqtt_ctrl->mqtt_cfg.credentials.client_id = luaL_checkstring(L, 2);
	mqtt_ctrl->mqtt_cfg.credentials.username = luaL_optstring(L, 3, "");
	mqtt_ctrl->mqtt_cfg.credentials.authentication.password = luaL_optstring(L, 4, "");
	return 0;
}

/*
mqtt心跳设置
@api mqttc:keepalive(time)
@int 可选 单位s 默认240s
@usage 
mqttc:keepalive(30)
*/
static int l_mqtt_keepalive(lua_State *L) {
	luat_mqtt_ctrl_t * mqtt_ctrl = get_mqtt_ctrl(L);
	mqtt_ctrl->mqtt_cfg.session.keepalive = luaL_optinteger(L, 2, 240);
	return 0;
}

/*
mqtt回调注册
@api mqttc:on(cb)
@function cb mqtt回调,参数包括mqtt_client, event, data, payload
@usage 
mqttc:on(function(mqtt_client, event, data, payload)
	-- 用户自定义代码
	log.info("mqtt", "event", event, mqtt_client, data, payload)
end)
*/
static int l_mqtt_on(lua_State *L) {
	luat_mqtt_ctrl_t * mqtt_ctrl = get_mqtt_ctrl(L);
	if (mqtt_ctrl->mqtt_cb != 0) {
		luaL_unref(L, LUA_REGISTRYINDEX, mqtt_ctrl->mqtt_cb);
		mqtt_ctrl->mqtt_cb = 0;
	}
	if (lua_isfunction(L, 2)) {
		lua_pushvalue(L, 2);
		mqtt_ctrl->mqtt_cb = luaL_ref(L, LUA_REGISTRYINDEX);
	}
	return 0;
}

/*
连接服务器
@api mqttc:connect()
@usage 
mqttc:connect()
*/
static int l_mqtt_connect(lua_State *L) {
	luat_mqtt_ctrl_t * mqtt_ctrl = get_mqtt_ctrl(L);
	mqtt_ctrl->client = esp_mqtt_client_init(&(mqtt_ctrl->mqtt_cfg));
    esp_mqtt_client_register_event(mqtt_ctrl->client, ESP_EVENT_ANY_ID, mqtt_event_handler, mqtt_ctrl);
	esp_mqtt_client_start(mqtt_ctrl->client);
	return 0;
}

/*
自动重连
@api mqttc:autoreconn(reconnect, reconnect_time)
@bool 是否自动重连
@int 自动重连周期 单位ms 默认3s
@usage 
mqttc:autoreconn(true)
*/
static int l_mqtt_autoreconn(lua_State *L) {
	luat_mqtt_ctrl_t * mqtt_ctrl = get_mqtt_ctrl(L);
	if (lua_isboolean(L, 2)){
		mqtt_ctrl->reconnect = lua_toboolean(L, 2);
	}
	mqtt_ctrl->reconnect_time = luaL_optinteger(L, 3, 3000);
	return 0;
}

/*
发布消息
@api mqttc:publish(topic, data, qos)
@string topic 主题
@string data  消息
@int qos 0/1/2 默认0
@return int message_id
@usage 
mqttc:publish("/luatos/123456", "123")
*/
static int l_mqtt_publish(lua_State *L) {
	uint16_t payload_len= 0;
	luat_mqtt_ctrl_t * mqtt_ctrl = get_mqtt_ctrl(L);
	const char * topic = luaL_checkstring(L, 2);
	const char * payload = luaL_checklstring(L, 3, &payload_len);
	uint8_t qos = luaL_optinteger(L, 4, 0);
	uint8_t retain = luaL_optinteger(L, 5, 0);
	int message_id = esp_mqtt_client_publish(mqtt_ctrl->client, topic, payload, payload_len, qos, retain);
	lua_pushinteger(L, message_id);
	return 1;
}

/*
mqtt客户端关闭(关闭后资源释放无法再使用)
@api mqttc:close()
@usage 
mqttc:close()
*/
static int l_mqtt_close(lua_State *L) {
	luat_mqtt_ctrl_t * mqtt_ctrl = get_mqtt_ctrl(L);
	esp_mqtt_client_stop(mqtt_ctrl->client);
	esp_mqtt_client_destroy(mqtt_ctrl->client);
	if (mqtt_ctrl->mqtt_cb != 0) {
		luaL_unref(L, LUA_REGISTRYINDEX, mqtt_ctrl->mqtt_cb);
		mqtt_ctrl->mqtt_cb = 0;
	}
	mqtt_release(mqtt_ctrl);
	return 0;
}

/*
mqtt客户端是否就绪
@api mqttc:ready()
@return bool 客户端是否就绪
@usage 
local error = mqttc:ready()
*/
static int l_mqtt_ready(lua_State *L) {
	luat_mqtt_ctrl_t * mqtt_ctrl = get_mqtt_ctrl(L);
	lua_pushboolean(L, mqtt_ctrl->mqtt_state);
	return 1;
}

static int _mqtt_struct_newindex(lua_State *L);

void luat_mqtt_struct_init(lua_State *L) {
    luaL_newmetatable(L, LUAT_MQTT_CTRL_TYPE);
    lua_pushcfunction(L, _mqtt_struct_newindex);
    lua_setfield( L, -2, "__index" );
    lua_pop(L, 1);
}

#include "rotable2.h"
static const rotable_Reg_t reg_mqtt[] =
{
	{"create",			ROREG_FUNC(l_mqtt_create)},
	{"auth",			ROREG_FUNC(l_mqtt_auth)},
	{"keepalive",		ROREG_FUNC(l_mqtt_keepalive)},
	{"on",				ROREG_FUNC(l_mqtt_on)},
	{"connect",			ROREG_FUNC(l_mqtt_connect)},
	{"autoreconn",		ROREG_FUNC(l_mqtt_autoreconn)},
	{"publish",			ROREG_FUNC(l_mqtt_publish)},
	{"subscribe",		ROREG_FUNC(l_mqtt_subscribe)},
	{"unsubscribe",		ROREG_FUNC(l_mqtt_unsubscribe)},
	{"close",			ROREG_FUNC(l_mqtt_close)},
	{"ready",			ROREG_FUNC(l_mqtt_ready)},

	{ NULL,             ROREG_INT(0)}
};

static int _mqtt_struct_newindex(lua_State *L) {
	rotable_Reg_t* reg = reg_mqtt;
    const char* key = luaL_checkstring(L, 2);
	while (1) {
		if (reg->name == NULL)
			return 0;
		if (!strcmp(reg->name, key)) {
			lua_pushcfunction(L, reg->value.value.func);
			return 1;
		}
		reg ++;
	}
    return 0;
}

LUAMOD_API int luaopen_mqtt( lua_State *L ) {
    luat_newlib2(L, reg_mqtt);
	luat_mqtt_struct_init(L);
    return 1;
}


