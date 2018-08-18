#ifndef __BT_ACCEPTOR_H_
#define __BT_ACCEPTOR_H_

#include "esp_spp_api.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "uart.h"

#define SPP_TAG "SPP_ACCEPTOR_DEMO"
#define SPP_SERVER_NAME "SPP_SERVER"
#define BT_DEVICE_NAME "globalpilotBluetooth"
#define SPP_SHOW_DATA 0
#define SPP_SHOW_SPEED 1
#define SPP_SHOW_MODE SPP_SHOW_DATA    /*Choose show mode: show data or speed*/

#if (SPP_SHOW_MODE == SPP_SHOW_DATA)
#define SPP_DATA_LEN 20
#else
#define SPP_DATA_LEN ESP_SPP_MAX_MTU
#endif

struct sys_data {
	int wifi_tcp_enabled;
	int wifi_udp_enabled;
	int bt_enabled;
};

#define DISABLE 0
#define ENABLE	1

extern esp_spp_cb_param_t *connector_data;
void bt_server_init(void);

#endif
