
#include "luat_base.h"
#include "luat_shell.h"
#include "luat_uart.h"

#include "luat_log.h"
#define LUAT_LOG_TAG "shell"

void luat_shell_write(char* buff, size_t len) {
	luat_uart_write(0, buff, len);
}

void luat_shell_notify_recv(void) {
}
