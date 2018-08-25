#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "cmd.h"
#include "esp_log.h"
#include "uart.h"

char *sys_tips = "\
Welcom to esp32 system!\n \
You can use these commands to change functions:\n\
1. set uart b [baudrate]\n \
2. set apmode ssid [WF01]\n \
3. set wlan join [4]\n \
4. set wlan chan [1]\n \
5. set ip address [192.168.1.1]\n\
6. set ip gateway [192.168.1.1]\n\
7. set ip netmask [255.255.255.0]\n\
8. set ip dhcp [4]\n \
9. hi\n	\
Have a nice day!\n";

char *cmd_entry = "$$$";
char *cmd_type[] = {"set uart b ",\
					"set wlan join ",\
					"set wlan chan ",\
					"set ip dhcp ",\
					"set apmode ssid ",\
					"set ip address",\
					"set ip gateway ",\
					"set ip netmask ",\
};
#define INT_TYPE_LEN	4

struct cmd_node {
	int current_state;
	char *data;
};

char *prompt = "\n\r$>";

int cmd_state = 0;

struct cmd_node cmd_data;

int get_cmd_header(const char *str)
{
	return 0;
}

int enter_cmd_state(const char *str, int len)
{
	if(!memcmp(str, cmd_entry, len)) {
		ESP_LOGE(TAG, "Enter command mode..\n");
		return 1;
	}
	return 0;
}

/*
 * set user data
 * str: string value which need to process
 * user_data: user data which need to set
 * return the cmd type.
 */
int cmd_set_user_data(const char *str, struct get_user_data *user_data)
{
	int len, i;
	char *char_data = NULL;
	int int_data = 0;
	char data_send[50];

	for(i=0; i<ARRAY_SIZE(cmd_type); i++) {
		len = strlen(cmd_type[i]);
		if(!memcmp(cmd_type[i], str, len)) {
			char_data = str + len;
			ESP_LOGE(TAG, "Got char data: %s\n", char_data);
			if(i < INT_TYPE_LEN) {
				int_data = atoi(char_data);
				ESP_LOGE(TAG, "Got int data: %d\n", int_data);
			}
			switch(i) {
			case UART:
				user_data->uart_buand = int_data;
				ESP_LOGE(TAG, "uart_buand: %d\n", user_data->uart_buand);
				sprintf(data_send, "uart_buand: %d\n", user_data->uart_buand);
				send(udp_socket, data_send, strlen(data_send), 0);
				break;
			case WIFI_JOIN:
				user_data->wifi_join = int_data;
				ESP_LOGE(TAG, "wifi_join: %d\n", user_data->wifi_join);
				break;
			case WIFI_CH:
				user_data->wifi_chan = int_data;
				ESP_LOGE(TAG, "wifi_chan: %d\n", user_data->wifi_chan);
				break;
			case WIFI_DHCP:
				user_data->wifi_dhcp = int_data;
				ESP_LOGE(TAG, "wifi_dhcp: %d\n", user_data->wifi_dhcp);
				break;
			case WIFI_SSID:
				user_data->wifi_ssid = char_data;
				ESP_LOGE(TAG, "wifi_ssid: %s\n", user_data->wifi_ssid);
				break;
			case WIFI_ADDR:
				user_data->wifi_address = char_data;
				ESP_LOGE(TAG, "wifi_address: %s\n", user_data->wifi_address);
				break;
			case WIFI_GW:
				user_data->wifi_gateway = char_data;
				ESP_LOGE(TAG, "wifi_gateway: %s\n", user_data->wifi_gateway);
				break;
			case WIFI_NM:
				user_data->wifi_netmask = char_data;
				ESP_LOGE(TAG, "wifi_netmask: %s\n", user_data->wifi_netmask);
				break;
			}
			return i;
		}
	}
	return -1;
}

void cmd_process(void *pvParameters)
{
	char databuff[1024];
	int len = 0;
	struct get_user_data *user_data = malloc(sizeof(struct get_user_data));

	ESP_LOGI(TAG, "%s\n", __func__);

	memset(databuff, EXAMPLE_PACK_BYTE_IS, sizeof(databuff));
	while(1) {
		memset(databuff, 0x00, sizeof(databuff));
		len = recv(udp_socket, databuff, sizeof(databuff), 0);
		cmd_set_user_data(databuff, user_data);
	}
}

int cmd_cli(struct cmd_ops *ops, const char *buff, int len)
{
	if(len) {
		memcpy(&ops->data[ops->len], buff, len);
		if(!memcmp("\r", &ops->data[ops->len], 1) || !memcmp("\n", &ops->data[ops->len], 1)) {
			/* send data and next prompt */
			if(ops->len) {
				ops->cmd_send(ops->port, "\n\r", 2);
				ops->cmd_send(ops->port, ops->data, ops->len);
			}
			ops->cmd_send(ops->port, prompt, strlen(prompt));
			memset(ops->data, 0x00, ops->len);
			ops->len = 0;
		}
		else {
			/* only uart need to send data back */
			if(ops->pdata)
				ops->cmd_send(ops->port, buff, len);
			ops->len += len;
		}
		if(ops->len > 1024)
			ops->len = 0;
	}
	return 0;
}
