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

char *prompt = "\n\r$>";

int cmd_state = 0;

struct cmd_node cmd_data;

int get_cmd_header(const char *str)
{
	return 0;
}

void cmd_entry(struct cmd_ops *ops, const char *str)
{
	static int cmd_entry_count = 0;
	char *cmd_entry_prompt = "\n\rEnter Command Mode\n\r";
	if(!memcmp(str, "$", 1)) {
		cmd_entry_count ++;
	}
	if(cmd_entry_count >= 3 && cmd_data.current_state != CMD_CMD) {
		cmd_data.current_state = CMD_CMD;
		ops->cmd_send(ops->port, cmd_entry_prompt, strlen(cmd_entry_prompt));
		cmd_entry_count = 0;
	}
}

static void cmd_exit(struct cmd_ops *ops)
{
	char *cmd_exit_prompt = "\n\rExit Command Mode\n\r";
	cmd_data.current_state = CMD_NORMAL;
	ops->cmd_send(ops->port, cmd_exit_prompt, strlen(cmd_exit_prompt));
}

static int cmd_save_user_data(struct get_user_data *user_data)
{
	esp_err_t err;

	if(user_data->wifi_ssid) {
		err = nvs_write_str(WIFI_SSID_ST_KEY, user_data->wifi_ssid);
		if(err != ESP_OK) {
			ESP_LOGI(TAG, "wifi ssid set error\n\r");
			return err;
		}
		/* free data */
		free(user_data->wifi_ssid);
		user_data->wifi_ssid = NULL;
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
int cmd_parse_user_data(const char *ops_data, struct get_user_data *user_data)
{
	int len, i;
	char *char_data = NULL;
	int int_data = 0;
	char data_send[50];

	struct cmd_ops *ops = container_of(ops_data, struct cmd_ops, data);

	for(i=0; i<ARRAY_SIZE(cmd_type); i++) {
		len = strlen(cmd_type[i]);
		if(!memcmp(cmd_type[i], ops_data, len)) {
			char_data = ops_data + len;
			ESP_LOGE(TAG, "Got char data: %s\n", char_data);
			ESP_LOGE(TAG, "Got char data len: %d\n", strlen(char_data));
			if(i < INT_TYPE_LEN) {
				int_data = atoi(char_data);
				ESP_LOGE(TAG, "Got int data: %d\n", int_data);
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
				strcpy(user_data->wifi_ssid, char_data);
				sprintf(data_send, "\n\rwifi_ssid: %s\n\r", user_data->wifi_ssid);
				ops->cmd_send(ops->port, data_send, strlen(data_send));
				break;
			case WIFI_ADDR:
				user_data->wifi_address = malloc(strlen(char_data));
				strcpy(user_data->wifi_address, char_data);
				sprintf(data_send, "\n\rwifi_address: %s\n\r", user_data->wifi_address);
				ops->cmd_send(ops->port, data_send, strlen(data_send));
				break;
			case WIFI_GW:
				user_data->wifi_gateway = malloc(strlen(char_data));
				strcpy(user_data->wifi_gateway, char_data);
				sprintf(data_send, "\n\rwifi_gateway: %s\n\r", user_data->wifi_gateway);
				ops->cmd_send(ops->port, data_send, strlen(data_send));
				break;
			case WIFI_NM:
				user_data->wifi_netmask = malloc(strlen(char_data));
				strcpy(user_data->wifi_netmask, char_data);
				sprintf(data_send, "\n\rwifi_netmask: %s\n\r", user_data->wifi_netmask);
				ops->cmd_send(ops->port, data_send, strlen(data_send));
				break;
			case SAVE:
				cmd_save_user_data(user_data);
				sprintf(data_send, "\n\rData saved\n\r");
				ops->cmd_send(ops->port, data_send, strlen(data_send));
				break;
			case EXIT:
				cmd_exit(ops);
				break;
			case REBOOT:
				esp_restart();
				break;
			default:
				break;
			}
			return i;
		}
	}
	sprintf(data_send, "\n\rCommand not found!\n\r");
	ops->cmd_send(ops->port, data_send, strlen(data_send));
	return -1;
}

struct get_user_data user_data;

typedef struct {
	char *cmd;
	char *data;
	int data_type;
	struct list_head *list;
}CMD_TYPE;

enum data_type {DATA_CHAR, DATA_INT};
struct list_head cmd_list_head;

CMD_TYPE wifi_ssid_cmd = {
		.cmd = "set apmode ssid ",
		.data_type = DATA_INT,
};

int cmd_register(CMD_TYPE *cmd)
{
	list_init(cmd->list);
	cmd->data = NULL;
	list_add_tail(&cmd_list_head, cmd->list);
	return 0;
}

int cmd_parse(struct list_head *head, char *buff)
{
	CMD_TYPE *node = NULL;
	char *get_data = NULL;
	int len = 0;
	while (head->next != NULL) {
		node = list_entry(head->next, CMD_TYPE, list);
		len = strlen(node->cmd);
		if (!memcmp(node->cmd, buff, len)) {
			get_data = buff + len;
			if(node->data_type == DATA_CHAR) {
				node->data = malloc(strlen(get_data));
				strcpy(node->data, get_data);
			}
			else {
				node->data = malloc(4);	//need 4 byte memory to save int data
				memcpy(node->data, get_data, 4);
			}
			return 0;
		}
	}
	return -1;
}

int cmd_save(struct list_head *head)
{
	CMD_TYPE *node = NULL;
	int len = 0;
	while (head->next != NULL) {
		node = list_entry(head->next, CMD_TYPE, list);
		if(!node->data) {
			if(node->data_type == DATA_CHAR) {
				err = nvs_write_str(WIFI_SSID_ST_KEY, node->data);
				if(err != ESP_OK) {
					ESP_LOGI(TAG, "wifi ssid set error\n\r");
					return err;
				}
			}
		}
	}
	return 0;
}

int cmd_init(void)
{
	memset(&user_data, 0x00, sizeof(struct get_user_data));
	memset(&cmd_data, 0x00, sizeof(struct cmd_node));
	cmd_register(&wifi_ssid_cmd);
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
		else {
			/* only uart need to send data back */
			if(ops->pdata)
				ops->cmd_send(ops->port, buff, len);
			ops->len -= len;
		}
		if(ops->len > 1024)
			ops->len = 0;
	}
	return 0;
}
