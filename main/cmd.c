#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "cmd.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "uart.h"
#include "wifi_bt_storage.h"

char *sys_tips = "\
\n\rWelcom to esp32 system!\n\r\
You can use these commands to change functions:\n\r\
1. set uart b [baudrate]\n\r\
2. set apmode ssid [WF01]\n\r\
3. set wlan join [4]\n\r\
4. set wlan chan [1]\n\r\
5. set ip address [192.168.1.1]\n\r\
6. set ip gateway [192.168.1.1]\n\r\
7. set ip netmask [255.255.255.0]\n\r\
8. set ip dhcp [4]\n\r\
9. save\n\r\
10. exit\n\r\
11. reboot\n\r\
12. hi\n\r\
Have a nice day!\n\r";

char *cmd_entry = "$$$";
char *cmd_type[] = {"set uart b ",\
					"set wlan join ",\
					"set wlan chan ",\
					"set ip dhcp ",\
					"set apmode ssid ",\
					"set ip address",\
					"set ip gateway ",\
					"set ip netmask ",\
					"save",\
					"exit",\
					"reboot"\
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

static int cmd_save_user_data(struct get_user_data *user_data)
{
	esp_err_t err;

	if(user_data->wifi_ssid) {
		err = nvs_write_str(WIFI_SSID_ST_KEY, user_data->wifi_ssid);
		if(err != ESP_OK) {
			ESP_LOGI(TAG, "wifi ssid erase error\n\r");
			return err;
		}
		err = nvs_write_str(WIFI_SSID_ST_KEY, WIFI_SSID_ST_KEY);
		if(err != ESP_OK) {
			ESP_LOGI(TAG, "wifi ssid set error\n\r");
			return err;
		}
	}
	if(user_data->wifi_pwd) {
		err = nvs_write_str(WIFI_PWD_ST_KEY, (const char *)user_data->wifi_pwd);
		if(err != ESP_OK) {
			ESP_LOGI(TAG, "wifi pwd set error\n\r");
			return err;
		}
	}
	if(user_data->wifi_join) {
		err = nvs_write_i32(WIFI_JOIN_ST_KEY, user_data->wifi_join);
		if(err != ESP_OK) {
			ESP_LOGI(TAG, "wifi join set error\n\r");
			return err;
		}
	}

	if(user_data->uart_buand) {
		err = nvs_write_i32(UART_STORAGE_KEY, user_data->uart_buand);
		if(err != ESP_OK) {
			ESP_LOGI(TAG, "uart set error\n\r");
			return err;
		}
	}

	return 0;
}

/*
 * set user data
 * str: string value which pointer to cmd_ops->data
 * user_data: user data which need to set
 * return the cmd type.
 */
static int cmd_parse_user_data(const char *str, struct get_user_data *user_data)
{
	int len, i;
	char *char_data = NULL;
	int int_data = 0;
	char data_send[50];

	struct cmd_ops *ops = container_of(str, struct cmd_ops, data);

	for(i=0; i<ARRAY_SIZE(cmd_type); i++) {
		len = strlen(cmd_type[i]);
		if(!memcmp(cmd_type[i], str, len)) {
			char_data = str + len;
			//ESP_LOGE(TAG, "Got char data: %s\n", char_data);
			if(i < INT_TYPE_LEN) {
				int_data = atoi(char_data);
				//ESP_LOGE(TAG, "Got int data: %d\n", int_data);
			}
			switch(i) {
			case UART:
				user_data->uart_buand = int_data;
				sprintf(data_send, "\n\ruart_buand: %d\n\r", user_data->uart_buand);
				ops->cmd_send(ops->port, data_send, strlen(data_send));
				break;
			case WIFI_JOIN:
				user_data->wifi_join = int_data;
				sprintf(data_send, "\n\rwifi_join: %d\n\r", user_data->wifi_join);
				ops->cmd_send(ops->port, data_send, strlen(data_send));
				break;
			case WIFI_CH:
				user_data->wifi_chan = int_data;
				sprintf(data_send, "\n\rwifi_chan: %d\n\r", user_data->wifi_chan);
				ops->cmd_send(ops->port, data_send, strlen(data_send));
				break;
			case WIFI_DHCP:
				user_data->wifi_dhcp = int_data;
				sprintf(data_send, "\n\rwifi_dhcp: %d\n\r", user_data->wifi_dhcp);
				ops->cmd_send(ops->port, data_send, strlen(data_send));
				break;
			case WIFI_SSID:
				user_data->wifi_ssid = malloc(strlen(char_data));
				memcpy(user_data->wifi_ssid, char_data, strlen(char_data));
				sprintf(data_send, "\n\rwifi_ssid: %s\n\r", user_data->wifi_ssid);
				ops->cmd_send(ops->port, data_send, strlen(data_send));
				break;
			case WIFI_ADDR:
				user_data->wifi_address = malloc(strlen(char_data));
				memcpy(user_data->wifi_address, char_data, strlen(char_data));
				sprintf(data_send, "\n\rwifi_address: %s\n\r", user_data->wifi_address);
				ops->cmd_send(ops->port, data_send, strlen(data_send));
				break;
			case WIFI_GW:
				user_data->wifi_gateway = malloc(strlen(char_data));
				memcpy(user_data->wifi_gateway, char_data, strlen(char_data));
				sprintf(data_send, "\n\rwifi_gateway: %s\n\r", user_data->wifi_gateway);
				ops->cmd_send(ops->port, data_send, strlen(data_send));
				break;
			case WIFI_NM:
				user_data->wifi_netmask = malloc(strlen(char_data));
				memcpy(user_data->wifi_netmask, char_data, strlen(char_data));
				sprintf(data_send, "\n\rwifi_netmask: %s\n\r", user_data->wifi_netmask);
				ops->cmd_send(ops->port, data_send, strlen(data_send));
				break;
			case SAVE:
				cmd_save_user_data(user_data);
				break;
			case EXIT:
				break;
			case REBOOT:
				esp_restart();
				break;
			}
			return i;
		}
	}
	return -1;
}

struct get_user_data user_data;

int cmd_init(void)
{
	memset(&user_data, 0x00, sizeof(struct get_user_data));
	return 0;
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
				/* parse data */
				if(!memcmp("hi", ops->data, 2))
					ops->cmd_send(ops->port, sys_tips, strlen(sys_tips));
				else cmd_parse_user_data(ops->data, &user_data);
			}
			ops->cmd_send(ops->port, prompt, strlen(prompt));
			memset(ops->data, 0x00, ops->len);
			ops->len = 0;
		}
		else if(memcmp("\b", buff, 1)){
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
