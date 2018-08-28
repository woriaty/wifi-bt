#ifndef _CMD_H_
#define _CMD_H_

#include "wifi_bt_storage.h"
#include "wifi_udp.h"
#include "esp_system.h"
#include "list.h"

#define TAG "$>"
#define ARRAY_SIZE(array)	sizeof(array)/sizeof(array[0])
#define TO_STR(a)			#a

struct get_user_data {
	int uart_buand;
	int wifi_join;
	int wifi_chan;
	int wifi_dhcp;
	char *wifi_ssid;
	char *wifi_pwd;
	char *wifi_address;
	char *wifi_gateway;
	char *wifi_netmask;
};

struct cmd_ops {
	int (*cmd_send)(int port, const char *buff, int len);
	char data[1024];
	int len;		/* data length */
	int port;
	char *pdata;	/* private data */
};

struct cmd_node {
	int current_state;
	char *data;
};

enum cmd_state { CMD_NORMAL, CMD_CMD };
enum data_type {UART, WIFI_JOIN, WIFI_CH, WIFI_DHCP, WIFI_SSID,\
				WIFI_ADDR, WIFI_GW, WIFI_NM, SAVE, EXIT, REBOOT};

extern char *prompt;
extern struct get_user_data user_data;
extern struct cmd_node cmd_data;

void cmd_entry(struct cmd_ops *ops, const char *str);
int cmd_cli(struct cmd_ops *ops, const char *buff, int len);
int cmd_init(void);


#endif
