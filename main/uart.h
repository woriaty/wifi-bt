#ifndef _UART_H_
#define _UART_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_spp_api.h"
#include "bt_server.h"
#include "driver/uart.h"
#include "esp_log.h"

#include "wifi_tcp.h"
#include "wifi_udp.h"

#define ECHO_TEST_TXD  (1)
#define ECHO_TEST_RXD  (3)
#define ECHO_TEST_RTS  (22)
#define ECHO_TEST_CTS  (19)

#define BUF_SIZE (1024)

#define UART_TAG	"UART"
#define UART_CONNECTED_BIT	BIT0

extern EventGroupHandle_t uart_event_group;

void uart_task(void *pvParameters);

#endif
