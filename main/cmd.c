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

char *prompt = "\n\r$>";

struct cmd_node cmd_data;

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

static void cmd_exit(struct list_head *head, const char *ops_data)
{
	struct cmd_ops *ops = container_of(ops_data, struct cmd_ops, data);
	char *cmd_exit_prompt = "\n\rExit Command Mode\n\r";

	cmd_data.current_state = CMD_NORMAL;
	ops->cmd_send(ops->port, cmd_exit_prompt, strlen(cmd_exit_prompt));
}

static void print_systips(struct list_head *head, const char *ops_data)
{
	struct cmd_ops *ops = container_of(ops_data, struct cmd_ops, data);

	ops->cmd_send(ops->port, sys_tips, strlen(sys_tips));
}

static void cmd_save(struct list_head *head, const char *ops_data);

static void cmd_reboot(struct list_head *head, const char *ops_data)
{
	esp_restart();
}

/* global list head */
struct list_head cmd_list_head;

struct cmd_table user_cmd_table[] = {
		{"set apmode ssid ", DATA_CHAR, WIFI_SSID_ST_KEY, NULL},
		{"set wlan join ", DATA_INT, WIFI_JOIN_ST_KEY, NULL},
		{"set wlan chan ", DATA_INT, WIFI_CHAN_ST_KEY, NULL},
		{"set ip dhcp ", DATA_INT, WIFI_DHCP_ST_KEY, NULL},
		{"set ip address ", DATA_CHAR, WIFI_ADDR_ST_KEY, NULL},
		{"set ip gateway ", DATA_CHAR, WIFI_GATE_ST_KEY, NULL},
		{"set ip netmask ", DATA_CHAR, WIFI_MASK_ST_KEY, NULL},
		{"set uart b ", DATA_INT, UART_STORAGE_KEY, NULL},
		{"hi", DATA_NONE, NULL, print_systips},
		{"save", DATA_NONE, NULL, cmd_save},
		{"reboot", DATA_NONE, NULL, cmd_reboot},
		{"exit", DATA_NONE, NULL, cmd_exit},
		{}
};

static int cmd_register(void)
{
	int i;
	for(i=0;i<ARRAY_SIZE(user_cmd_table);i++) {
		CMD_TYPE *user_cmd = malloc(sizeof(CMD_TYPE));
		if(!user_cmd) {
			return -1;
		}
		if(!user_cmd_table[i].cmd)
			return -1;
		user_cmd->cmd = user_cmd_table[i].cmd;
		user_cmd->data_type = user_cmd_table[i].data_type;
		user_cmd->id = user_cmd_table[i].id;
		if(user_cmd_table[i].pfn)
			user_cmd->pfn = user_cmd_table[i].pfn;

		user_cmd->data = NULL;

		list_init(&user_cmd->list);
		list_add_tail(&cmd_list_head, &user_cmd->list);
	}
	return 0;
}

static int cmd_parse(struct list_head *head, const char *ops_data)
{
	CMD_TYPE *node = NULL;
	char *get_data = NULL;
	int len = 0;
	struct list_head *cmd_list = head;

	while (cmd_list->next != NULL) {
		cmd_list = cmd_list->next;
		node = list_entry(cmd_list, CMD_TYPE, list);
		len = strlen(node->cmd);
		if (!memcmp(node->cmd, ops_data, len)) {
			get_data = ops_data + len;
			if (node->data_type != DATA_NONE) {
				node->data = malloc(strlen(get_data));
				strcpy(node->data, get_data);
			}

			if (node->pfn)
				node->pfn(head, ops_data);
			return 0;
		}
	}
	return -1;
}

static void cmd_save(struct list_head *head, const char *ops_data)
{
	CMD_TYPE *node = NULL;
	esp_err_t err;
	int value = 0;
	struct cmd_ops *ops = container_of(ops_data, struct cmd_ops, data);
	char echo[25];

	while (head->next != NULL) {
		head = head->next;
		node = list_entry(head, CMD_TYPE, list);
		if(node->data) {
			if(node->data_type == DATA_CHAR) {
				/* key need a string without spaces */
				err = nvs_write_str(node->id, node->data);
			}
			else {
				value = atoi(node->data);
				err = nvs_write_i32(node->id, value);
			}
			if(err != ESP_OK) {
				ESP_LOGI(TAG, "\n\r%s: error\n\r", node->cmd);
			}
			sprintf(echo, "\n\r%sto %s\n\r", node->cmd, node->data);
			ops->cmd_send(ops->port, echo, strlen(echo));
		}
	}
}

int cmd_init(void)
{
	memset(&cmd_data, 0x00, sizeof(struct cmd_node));
	list_init(&cmd_list_head);
	cmd_register();
	return 0;
}

/* */
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
				cmd_parse(&cmd_list_head, ops->data);
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
